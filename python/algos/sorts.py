#!/usr/bin/python2

toSort = [ 2, 4, 7, 1, 6, 5 ]
toSort2 = [ 9, 3, 1, 8, 2, 0 ]

done = 1

def quicksort(lst, p, r):
  if p < r:
    q = _partition(lst, p, r)
    quicksort(lst, p, q-1)
    quicksort(lst, q+1, r)


def _partition(lst, p, r):
  piv = lst[r]
  i = p-1
  j = p
  while j < r:
    if lst[j] <= piv:
      i = i + 1
      lst[i], lst[j] = lst[j], lst[i]
    j += 1
  lst[i+1], lst[r] = lst[r], lst[i+1]
  return i+1


if __name__ == '__main__':
  quicksort(toSort, 0 , len(toSort) -1)
  print toSort
  quicksort(toSort2, 0, len(toSort2) -1)
  print toSort2
