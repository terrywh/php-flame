#ifndef FLAME_CORE_CONFIG_H
#define FLAME_CORE_CONFIG_H

#if defined(__GNUC__) && __GNUC__ >= 4
# define FLAME_PHP_EXPORT __attribute__ ((visibility("default")))
#else
# define FLAME_PHP_EXPORT
#endif

#endif // FLAME_CORE_CONFIG_H
