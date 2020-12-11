# HashMap-172
A hash table based implementation of the string-to-int associative array written in **C** by 172nd group.

Instantiating:
```c
HM172Map *map = hm172_new_map(init_capacity, load_factor, hash_function);
```
Associating `key` with `value`:
```c
hm172_put(map, key, value);
if (!hm172_is_ok(map)) {
    HM172Status error = hm172_get_status(map);
    // handle error
}
```
Getting `value` by `key`:
```c
HM172Value *value = hm172_get(map, key);
if (value != NULL) {
    // use *value
}
```
Iterating over entries:
```c
HM172EntryIterator *iterator = hm172_get_entry_iterator(map);
if (!hm172_is_ok(map)) {
    HM172Status error = hm172_get_status(map);
    // handle error
}
HM172Entry *entry;
while ((entry = hm172_next_entry(iterator)) != NULL) {
    // use entry
}
hm172_free_entry_iterator(iterator);
```
Printing statistics to `stream`:
```c
hm172_fprint_stats(map, stream);
if (!hm172_is_ok(map)) {
    HM172Status error = hm172_get_status(map);
    // handle error
}
```
Freeing:
```c
hm172_free(map);
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
if (hm172_log_on_error(map)) {
    // handle error
}
```

</td>
<td>

```c
if (!hm172_is_ok(map)) {
    HM172Status error = hm172_get_status(map);
    hm172_log_status_on_error(&error, "map");
    // handle error
}
```

</td>
</tr>
<tr>
<td> Auto logging and freeing </td>
<td>

```c
if (hm172_log_and_free_on_error(map)) {
    // handle error
}
```

</td>
<td>

```c
if (hm172_log_on_error(map)) {
    hm172_free(map);
    // handle error
}
```

</td>
</tr>
</table>