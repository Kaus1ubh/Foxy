#ifndef ALIAS_H
#define ALIAS_H

void alias_init();
int alias_add(const char *name, const char *value);
int alias_remove(const char *name);
const char *alias_resolve(const char *name);
void alias_print_all();

#endif // ALIAS_H
