#ifndef FOREACH_H

#ifndef FOREACH_LEN_CALC
//#error 
#endif//FOREACH_LEN_CALC

#ifndef CAT
#define CAT_EXPAND(a, b) a ## b
#define CAT(a, b) CAT_EXPAND(a, b)
#endif//CAT

#ifndef FOR_EACH_INDEX_TYPE
#define FOR_EACH_INDEX_TYPE uint
#endif//FOR_EACH_INDEX_TYPE

#define FOR_EACH_ONCE(...) for(FOR_EACH_INDEX_TYPE __VA_ARGS__, FOR_EACH_ONCE_TEST)
#define FOR_EACH_ONCE_TEST \
		CAT(foreach_once,__LINE__) = 1;\
		CAT(foreach_once,__LINE__);\
		CAT(foreach_once,__LINE__) = 0\

// index and value (not pointer), updates if length changes
// NOTE: nothing needs to be done if array is appended to.
// if it's inserted one before current point, decrement the index
// the index for Value is i_Value
// (or could return to defining name manually)
#define foreach(valtype, val, arena)         \
	FOR_EACH_ONCE(CAT(i,val) = 0)            \
                                             \
		for(valtype val = (arena).Items[0];  \
			CAT(i,val) < Len(arena);         \
			val = (arena).Items[++CAT(i,val)])

// index and value (not pointer), length fixed
#define foreachf(valtype, val, arena)               \
	FOR_EACH_ONCE(CAT(i,val) = 0,                   \
			CAT(foreach_len,__LINE__) = (FOR_EACH_INDEX_TYPE)Len(arena))  \
                                                    \
		for(valtype val = (arena).Items[0];         \
			CAT(i,val) < CAT(foreach_len,__LINE__); \
			val = (arena).Items[++CAT(i,val)])

// index and value (not pointer), length fixed, goes in reverse
#define foreachr(valtype, val, arena)         \
	FOR_EACH_ONCE(CAT(i,val) = (FOR_EACH_INDEX_TYPE)Len(arena))  \
                                              \
		for(valtype val = (arena).Items[0];   \
			CAT(i,val);                       \
			val = (arena).Items[--CAT(i,val)])

// index and pointer, length updates
#define foreachp(valtype, val, arena)          \
	FOR_EACH_ONCE(CAT(i,val) = 0)   \
                                               \
		for(valtype val = (arena).Items;       \
			CAT(i,val) < Len(arena);          \
			val = &(arena).Items[++CAT(i,val)])

// index and pointer, length fixed at start
#define foreachpf(valtype, val, arena) \
	FOR_EACH_ONCE(i = 0) \
for(valtype val = (arena).Items, \
			*CAT(foreach_end,__LINE__) = (valtype)( ((unsigned char *)(arena).Items) + (arena).Used ); \
		val < CAT(foreach_end,__LINE__); \
		++i, ++val)

// just pointer, length updates
#define forall(valtype, val, arena) \
	for(valtype *val = (arena).Items; \
			val < (arena).Items + Len(arena); \
			++val)

// just pointer, length fixed at start
#define forallf(valtype, val, arena) \
	for(valtype *val = (arena).Items, \
				*CAT(foreach_end,__LINE__) = (valtype)( ((unsigned char *)(arena).Items) + (arena).Used ); \
			val < CAT(foreach_end,__LINE__); \
			++val)

// *CAT(foreach_end,__LINE__) = (arena).Items + Len(Arena);

#define FOREACH_H
#endif//FOREACH_H
