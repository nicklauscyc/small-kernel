/** @file safe_strcmp.c
 *	@brief a safe version of strcmp
 *
 *	From score.c test case
 */

#include <string.h> /* strcmp() */
#include <stddef.h> /* NULL */

/** @brief Careful version of strcmp that avoids detrimental NULL dereferences
 *
 * Returns 0 if x == NULL, y == NULL
 * Returns some unspecified non-zero value if only one of x or y is NULL
 * Otherwise, returns strcmp(x, y)
 *
 * @param x string 1
 * @param y string 2
 * @return 0 if x equals y || x and y both NULL, non zero otherwise
 */
int
safe_strcmp(char *x, char *y)
{
    if (x == NULL && y == NULL) {
		return 0;
    } else if (x == NULL) {
		return -42;
    } else if (y == NULL) {
		return -42;
    } else {
        return strcmp(x, y);
	}
}




