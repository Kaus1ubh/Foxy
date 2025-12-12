#ifndef INTERACTION_H
#define INTERACTION_H

void history_init();
void history_load();
void add_to_history(const char *cmd);
int read_line_with_history(char *buf, int max_len);

#endif // INTERACTION_H
