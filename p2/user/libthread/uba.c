
#include <thr_internals.h>
#include <stddef.h> /* NULL */
#include <malloc.h> /* malloc() */
#include <assert.h> /* assert() */

/*********************************************************************/
/*                                                                   */
/* Internal helper functions                                         */
/*                                                                   */
/*********************************************************************/

/** @brief Checks invariants for unbounded arrays
 *
 *  The invariants here are implemented as asserts so in the even that
 *  an assertion fails, the developer knows exactly which assertion
 *  fails. There are many invariants since on top of being an
 *  unbounded array, it is also a circular array.
 *
 *  @param arr Pointer to uba to be checked
 *  @return 1 if valid uba pointer, 0 otherwise
 */
int is_uba(uba *arr) {

  /* non-NULL check */
  assert(arr != NULL);

  /* size check */
  assert(0 <= arr->size && arr->size < arr->limit);

  /* limit check */
  assert(0 < arr->limit);

  /* first and last check */
  if (arr->first <= arr->last) {
    assert(arr->size == arr->last - arr->first);
  } else {
    assert(arr->size == arr->limit - arr->first + arr->last);
  }
  /* first and last in bounds check */
  assert(0 <= arr->first && arr->first < arr->limit);
  assert(0 <= arr->last && arr->last < arr->limit);

  /* data non-NULL check */
  assert(arr->data != NULL);
  return 1;
}

/** @brief Initializes a uba and returns a pointer to it
 *
 *  Fatal errors are only thrown if size == 0 or size == 1 and yet
 *  init_uba() returns NULL (out of heap space)
 *
 *  @param type Element type in the uba
 *  @param limit Actual size of the uba.
 *  @return Pointer to valid uba if successful, NULL if malloc() or
 *          calloc() fails or if limit < 0
 */
uba *uba_new(int limit) {
  if (limit <= 0 ) return NULL;

  /* Convert limit to unsigned */
  uint32_t _limit = (uint32_t) limit;
  assert(0 <= _limit);

  /* Allocate memory for uba struct */
  uba *new_ubap = malloc(sizeof(uba));
  assert(new_ubap != NULL);
  if (new_ubap == NULL) return NULL;

  /* Allocate memory for the array */
  void *data = NULL;
  data = calloc(_limit, sizeof(uint8_t));
  assert(data != NULL);
  if (data == NULL) return NULL;

  /* Set fields of unbounded array uba */
  new_ubap->size = 0;
  new_ubap->limit = _limit;
  new_ubap->first = 0;
  new_ubap->last = 0;
  new_ubap->data = data;

  return new_ubap;
}

/** @brief Resizes the unbounded array
 *
 *  @param arr Pointer to a uba
 *  @param Pointer to the original uba if no resize needed, bigger uba o/w.
 */
uba *uba_resize(uba *arr) {
  assert(is_uba(arr));
  if (arr->size == arr->limit) {

    /* Only resize if sufficient */
    if (arr->size < UINT32_MAX / 2) {
      uba *new_arr = uba_new(arr->size * 2);
      assert(is_uba(new_arr));

      /* Copy elements over */
      while(arr->size > 0) {
        uint8_t elem = uba_rem(arr);
        uba_add(new_arr, elem);
      }
      /* Free the old array */
      free(arr->data);
      free(arr);

      /* Return new array */
      return new_arr;
    }
    /* at limit but cannot resize, OK for now but error will be thrown
     * if want to add a new element to arr
     */
  }
  return arr;
}

/** @brief Adds new element to the array and resizes if needed
 *
 *  @param arr Unbounded array pointer we want to add to
 *  @param elem Element to add to the unbounded array
 *  @return Void.
 */
void uba_add(uba *arr, uint8_t elem) {
  assert(is_uba(arr));
  assert(arr->size < arr->limit); /* Total memory is 256 MiB and since
                                     each array element is 1 byte we
                                     will never reach max possible limit
                                     of 2^32 */

  /* Insert at index one after last element in array */
  arr->data[arr->last] = elem;

  /* Update last index and increment size*/
  arr->last = (arr->last + 1) % arr->limit;
  arr->size += 1;
  assert(is_uba(arr));

  /* Resize if necessary */
  arr = uba_resize(arr);
  assert(is_uba(arr));
}

/** @brief Removes first character of the uba
 *  @param arr Pointer to uba
 *  @return First character of the uba
 */
uint8_t uba_rem(uba *arr) {
  assert(is_uba(arr));

  /* Get first element and 'remove' from the array, decrement size */
  uint8_t elem = arr->data[arr->first];
  arr->first = (arr->first + 1) % arr->limit;
  arr->size -= 1;
  assert(is_uba(arr));

  /* Return 'popped off' element */
  return elem;
}

/** @brief Checks if uba is empty
 *  @param arr Pointer to uba
 *  @return 1 if empty, 0 otherwise.
 */
int uba_empty(uba *arr) {
  assert(is_uba(arr));
  return arr->size == 0;
}

