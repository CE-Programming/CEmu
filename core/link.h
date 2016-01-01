#ifndef LINK_H
#define LINK_H

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct link_state {
    bool is_sending;
    bool is_recieving;
    bool sending;
    bool recieving;
    std::string current_file;
} link_state_t;

extern link_state_t link;

bool sendVariableLink(void);

#ifdef __cplusplus
}
#endif

#endif
