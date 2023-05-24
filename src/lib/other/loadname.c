#include <lib.h>
#include <string.h>

void _loadName(const char *name, Message *msg) {
/* This function is used to load a string into a type m3 message. If the
 * string fits in the message, it is copied there. If not, a pointer to
 * it is passed.
 */
	register size_t k;

	k = strlen(name) + 1;
	msg->m3_i1 = k;
	msg->m3_p1 = (char *) name;
	if (k <= sizeof(msg->m3_ca1))
	  strcpy(msg->m3_ca1, name);
}
