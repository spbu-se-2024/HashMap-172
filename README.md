# HashMap-172
A hash table based implementation of the string-to-int associative array written in **C** by 172nd group.

Instantiating:
```c
MAP *map = new_MAP(init_capacity, load_factor, hash_function);
```
Associating `key` with `value`:
```c
MAP_put(map, key, value);
if (!MAP_is_ok(map)) {
    STATUS error = MAP_get_status(map);
    // handle error
}
```
Getting `value` by `key`:
```c
map_value_t *value = MAP_get(map, key);
if (value != NULL) {
    // use *value
}
```
Iterating over entries:
```c
ENTRY_ITERATOR *iterator = MAP_get_entry_iterator(map);
if (!MAP_is_ok(map)) {
    STATUS error = MAP_get_status(map);
    // handle error
}
ENTRY *entry;
while ((entry = ENTRY_ITERATOR_next(iterator)) != NULL) {
    // use entry
}
ENTRY_ITERATOR_free(iterator);
```
Printing statistics to `stream`:
```c
MAP_fprint_stats(map, stream);
if (!MAP_is_ok(map)) {
    STATUS error = MAP_get_status(map);
    // handle error
}
```
Freeing:
```c
MAP_free(map);
```
Error handling shortcuts:

<table>
<tr>
<td> Description </td> <td> Shortcut </td> <td> Equivalent </td>
</tr>
<tr>
<td> Auto logging </td>
<td>

```c
if (MAP_log_on_error(map)) {
    // handle error
}
```

</td>
<td>

```c
if (!MAP_is_ok(map)) {
    STATUS error = MAP_get_status(map);
    STATUS_log_on_error(&error, "map");
    // handle error
}
```

</td>
</tr>
<tr>
<td> Auto logging and freeing </td>
<td>

```c
if (MAP_log_and_free_on_error(map)) {
    // handle error
}
```

</td>
<td>

```c
if (MAP_log_on_error(map)) {
    MAP_free(map);
    // handle error
}
```

</td>
</tr>
</table>