#ifndef FILE_MINIPKG2_GIT_H
#define FILE_MINIPKG2_GIT_H
#include <stdbool.h>

// Get the version of git.
char* git_version(void);

// Clone a git repository.
bool git_clone(const char* url, const char* dest, const char* branch);

// Synchronize a local git repository.
bool git_pull(const char* repo);

// Select a different branch.
bool git_checkout(const char* repo, const char* branch);

// Get the current branch.
char* git_branch(const char* repo);

// If the repo already exists, git_pull(dest), otherwise git_clone(url, dest, NULL);
bool git_get(const char* url, const char* dest);

#endif /* FILE_MINIPKG2_GIT_H */
