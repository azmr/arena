#ifndef ARENA_H
#ifndef ARENA_COPY
#define ARENA_COPY(dst, src, n) memcpy(dst, src, n)
#endif//ARENA_COPY

// DEBUG //////////////////////////////////////////////////////////////////////
// IMPORTANT
#ifdef MEMORY_DEBUG
#define MEMORY_DEBUG_INFO , __LINE__, __FILE__
#define MEMORY_DEBUG_INFO_ARGS , u32 _LINE_, char *_FILE_
#define MEMORY_ARENA_DEBUG_INFO memory_debug_info DebugInfo;
#define UPDATE_MEMORY_ARENA_DEBUG_INFO(Arena, _LINE_, _FILE_) ++Arena->DebugInfo.NumAllocs;\
												Arena->DebugInfo.LineNumbers[Arena->DebugInfo.NumAllocs] = _LINE_;\
												Arena->DebugInfo.Filenames[Arena->DebugInfo.NumAllocs] = _FILE_
#define MAX_DEBUG_ALLOCATIONS 256
typedef struct memory_debug_info
{
	memory_index NumAllocs;
	u32 LineNumbers[MAX_DEBUG_ALLOCATIONS];
	char *Filenames[MAX_DEBUG_ALLOCATIONS];
	/* char *FunctionNames[MAX_DEBUG_ALLOCATIONS]; */
} memory_debug_info;

#else // #ifdef MEMORY_DEBUG
#define MEMORY_DEBUG_INFO
#define MEMORY_DEBUG_INFO_ARGS
#define MEMORY_ARENA_DEBUG_INFO
#define UPDATE_MEMORY_ARENA_DEBUG_INFO(Arena, _LINE_, _FILE_)
#define MAX_DEBUG_ALLOCATIONS
#endif // #ifdef MEMORY_DEBUG
///////////////////////////////////////////////////////////////////////////////

#define MEMORY_ARENA_MEMBERS(extra_member) \
	MEMORY_ARENA_DEBUG_INFO \
	memory_index Size; \
	memory_index Used; \
	union { \
		unsigned char *Bytes; \
		void *Base; \
		extra_member \
	}

typedef struct memory_arena
{
	MEMORY_ARENA_MEMBERS(void *Base_Untyped;);
} memory_arena;

#define arena_type(type) \
union type ## _arena { \
	memory_arena Arena; \
	struct{ MEMORY_ARENA_MEMBERS(type *Items;); }; \
} type ## _arena

/// Should already be cleared to 0 in initial allocation of memory
static inline void
InitArena(memory_arena *Arena, void *Base, memory_index Size)
{
	Arena->Size   = Size;
	Arena->Base   = Base;
	Arena->Used   = 0;
}

static inline memory_arena
CreateArena(void *Base, memory_index Size)
{
	memory_arena Result;
	Result.Size = Size;
	Result.Base = Base;
	Result.Used = 0;
	return Result;
}

#define ArenaCount(Arena, type) ((Arena).Used / sizeof(type))

static inline memory_index Len_(memory_arena Arena, memory_index ItemSize)
{
	Assert((Arena.Used % ItemSize) == 0);
	return Arena.Used / ItemSize;
}
static inline memory_index Cap_(memory_arena Arena, memory_index ItemSize)
{
	Assert((Arena.Size % ItemSize) == 0);
	return Arena.Size / ItemSize;
}
// TODO: see whether these sizeofs evaluate -> side effects
// could add ItemSize element to prevent multiple evaluation...
// TODO: assert &arena to cover most cases of 
#define Len(a) (SRAssert(sizeof(*(a).Items)), Len_(*(memory_arena *)&(a), sizeof(*(a).Items)))
#define Cap(a) (SRAssert(sizeof(*(a).Items)), Cap_(*(memory_arena *)&(a), sizeof(*(a).Items)))

// Type punning can be done at the user level
#define SRAssert_MEM_ARENA(arena) SRAssert(sizeof(*(arena))==sizeof(memory_arena))
#define SRAssert_ArenaPush(arena, el) SRAssert(sizeof(*(arena)->Items)==sizeof(el))

#define ArenaAddressToIndex(arena, address) (((unsigned char *)(address) - (arena)->Bytes) / sizeof(*(arena)->Items))
#define ArenaIndex(arena, address) ((address - (arena)->Items))

// TODO IMPORTANT: make threadsafe
#define GrowSize(arena, size) (SRAssert_MEM_ARENA(arena), GrowSize_((memory_arena*)(arena), size MEMORY_DEBUG_INFO))
#define GrowStruct(arena, type)   ((type *)GrowSize(arena,     sizeof(type)))
#define GrowArray(arena, type, n) ((type *)GrowSize(arena, (n)*sizeof(type)))

// for non-typed memory arenas
#define AppendStruct(arena, type, Struct) (*GrowStruct(arena, type) = (Struct))
#define AppendSize(arena, ptr, cBytes) Copy(cBytes, ptr, GrowSize(arena, cBytes))
#define AppendArr(arena, arr, n) AppendSize(arena, arr, (n)*sizeof(*(arr)))
#define AppendArray(arena, arr)  AppendSize(arena, arr, sizeof(arr))

// for typed memory arenas
#define PushN(arena, n) ArenaAddressToIndex(arena, GrowSize(arena, (n)*sizeof(*(arena)->Items)))
#define Push1(arena)    ArenaAddressToIndex(arena, GrowSize(arena,     sizeof(*(arena)->Items)))
#define Push(arena, el)        (SRAssert_ArenaPush(arena, el), (arena)->Items[Push1(arena)] = (el))
#define PushArr(arena, arr, n) (SRAssert_ArenaPush(arena, el), ArenaAddressToIndex(arena, AppendArr(arena, arr, n)))
#define PushArray(arena, arr)  (SRAssert_ArenaPush(arena, el), ArenaAddressToIndex(arena, AppendArray(arena, arr)))

static inline void *
GrowSize_(memory_arena *Arena, memory_index Size
			MEMORY_DEBUG_INFO_ARGS)
{
	Assert((Arena->Used + Size) <= Arena->Size);
	UPDATE_MEMORY_ARENA_DEBUG_INFO(Arena, _LINE_, _FILE_);
	void *Result = Arena->Bytes + Arena->Used;
	Arena->Used += Size;
	return Result;
}
#if 0
static inline memory_index
GrowSizeIndex_(memory_arena *Arena, memory_index Size, memory_index ElSize, 
	MEMORY_DEBUG_INFO_ARGS)
{
Assert((Arena->Used + Size) <= Arena->Size);
UPDATE_MEMORY_ARENA_DEBUG_INFO(Arena, _LINE_, _FILE_);
memory_index Result = Arena->Used / ElSize;
Arena->Used += Size;
return Result;
}
#endif

// needed to prevent multiple evaluation of index (e.g. if ++i)
#if     ARENA_NO_BOUNDS_CHECK
#define ArenaBoundsCheck(i, length) (i)
#endif//ARENA_NO_BOUNDS_CHECK
static inline memory_index
ArenaBoundsCheck(memory_index i, memory_index Length)
{
	Assert(i > 0);
	Assert(i < Length);
	return i;
}

#define PullStruct(arena, iEl, type) (type *)PullStruct_(*(memory_arena*)&(arena), (iEl), sizeof(type))
#define Pull(arena, iEl) ((arena).Items[ArenaBoundsCheck(iEl, Len(arena))])
static inline void *
PullStruct_(memory_arena Arena, memory_index ElNum, memory_index ElSize)
{
	memory_index Offset = ElNum * ElSize;
	Assert(Offset <= Arena.Used - ElSize);
	Assert(Offset >= 0);
	void *Result = Arena.Bytes + Offset;
	return Result;
}

#define RemoveStruct(Arena, type) *(type *)PopStruct_((memory_arena*)(Arena), sizeof(type))
#define PopDiscard(arena) ((arena)->Used -= sizeof(*(arena)->Items))
#define Pop(arena) (PopDiscard(arena), Pull(*(arena), Len(*(arena))))
// very much not threadsafe, should be immediately dereferenced to a type
static inline void *
PopStruct_(memory_arena *Arena, memory_index ElSize)
{
	Assert(Arena->Used >= ElSize);
	Arena->Used -= ElSize;
	void *Result = Arena->Bytes + Arena->Used;
	return Result;
}

static inline memory_arena
ArenaMalloc(memory_index Size)
{
	memory_arena Result;
	Result.Base   = malloc(Size);
	Result.Size   = Size;
	Result.Used   = 0;
	return Result;
}

static inline memory_arena
ArenaCalloc(memory_index Size)
{
	memory_arena Result;
	Result.Base   = calloc(Size, 1);
	Result.Size   = Size;
	Result.Used   = 0;
	return Result;
}

/// returns 1 for success. If fails, Arena is unchanged
static inline int
ArenaRealloc(memory_arena *Arena, memory_index Size)
{
	void *NewBase = realloc(Arena->Base, Size);
	Assert(NewBase);
	int Result = NewBase != 0;
	if(Result)
	{
		Arena->Base = NewBase;
		Arena->Size = Size;
	}
	return Result;
}

// TODO (from HMH 179):
/* InitializeArena(memory_arena *Arena, memory_index Size, void *Base) -> temporary */
/* GetAlignmentOffset(memory_arena *Arena, memory_index Alignment) */
/* GetArenaSizeRemaining(memory_arena *Arena, memory_index Alignment = 4) */
/* PushSize_(memory_arena *Arena, memory_index SizeInit, memory_index Alignment = 4) -> alignment*/
/* PushString(memory_arena *Arena, char *Source) */
/* BeginTemporaryMemory(memory_arena *Arena) */
/* CheckArena(memory_arena *Arena) */
/* SubArena(memory_arena *Result, memory_arena *Arena, memory_index Size, memory_index Alignment = 16) */
/* ZeroSize(memory_index Size, void *Ptr) */

static inline void
CopyArenaContents(memory_arena Src, memory_arena *Dst)
{
	Assert(Dst->Size >= Src.Used);
	ARENA_COPY(Dst->Base, Src.Base, Src.Used);
	Dst->Used = Src.Used;
}

#define ARENA_H
#endif//ARENA_H
