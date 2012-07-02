/*
 * Today's hack: quantum tunneling in structs
 *
 * 'entries' and 'term' are never anywhere referenced by word in code. In fact,
 * they serve as the hanging-off data accessed through repl.data[].
 */

#include <linux/valign.h>

#define xt_alloc_initial_table(type, typ2) ({ \
	unsigned int hook_mask = info->valid_hooks; \
	unsigned int nhooks = hweight32(hook_mask); \
	unsigned int bytes = 0, hooknum = 0, i = 0; \
	int replsize = paddedsize(0, 1, \
		struct type##_replace, struct type##_standard); \
	int entsize = paddedsize(replsize, nhooks, \
		struct type##_standard, struct type##_error); \
	int termsize = paddedsize(replsize+entsize, 1, \
		struct type##_error, int); \
	struct type##_replace *repl = kzalloc(replsize+entsize+termsize, \
		GFP_KERNEL); \
	if (repl == NULL) \
		return NULL; \
	struct type##_standard *entries = paddedstart(repl, replsize, \
		struct type##_standard); \
	struct type##_error *term = paddedstart(entries, entsize, \
		struct type##_error); \
	strncpy(repl->name, info->name, sizeof(repl->name)); \
	*term = (struct type##_error)typ2##_ERROR_INIT;  \
	repl->valid_hooks = hook_mask; \
	repl->num_entries = nhooks + 1; \
	repl->size = entsize+termsize; \
	for (; hook_mask != 0; hook_mask >>= 1, ++hooknum) { \
		if (!(hook_mask & 1)) \
			continue; \
		repl->hook_entry[hooknum] = bytes; \
		repl->underflow[hooknum]  = bytes; \
		entries[i++] = (struct type##_standard) \
			typ2##_STANDARD_INIT(NF_ACCEPT); \
		bytes += sizeof(struct type##_standard); \
	} \
	repl; \
})
