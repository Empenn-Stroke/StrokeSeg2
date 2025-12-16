#pragma once

/**
 * @brief stringification macro (no macro expansion).
 *
 * @par STRINGIFY_IMPL(x) will stringify whatever argument x you pass to it, verbatim.
 * Unfortunately, \# stringification just puts quotes around the character sequence that's directly
 * after the symbol. For this reason, STRINGIFY_IMPL should be avoided, and using STRINGIFY() should
 * be preferred.
 */
#define STRINGIFY_IMPL(char_seq) #char_seq

/**
 * @brief stringification macro (with macro expansion).
 */
#define STRINGIFY(macro_name) STRINGIFY_IMPL(macro_name)