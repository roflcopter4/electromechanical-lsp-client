#include "Common.hh"
#include <msgpack.hpp>

#include <cstdarg>

/*
 * This file very hastily converted from C. Perhaps I shouldn't have even bothered.
 */

inline namespace emlsp {
namespace ipc::mpack {

typedef unsigned char uchar;

union mpack_argument {
      bool         boolean;
      int64_t      num;
      uint64_t     uint;
      std::string *str;
      char        *c_str;
};

enum encode_fmt_next_type { OWN_VALIST, OTHER_VALIST, ARG_ARRAY };

#define NOP ((void)0)

#define NEW_STACK(TYPE, NAME)             \
      struct {                            \
            unsigned ctr;                 \
            TYPE     arr[128];            \
      } NAME = {}

#define POP(STACK) \
        ( (((STACK).ctr == 0) ? abort() : NOP), ((STACK).arr[--(STACK).ctr]) )

#define PUSH(STACK, VAL) \
        ( (STACK).arr[(STACK).ctr++] = (VAL) )

#define PEEK(STACK) \
        ( (((STACK).ctr == 0) ? abort() : NOP), ((STACK).arr[(STACK).ctr - 1U]) )

#define RESET(STACK) \
        ( (STACK).ctr = 0, (STACK).arr[0] = 0 )

#define STACK_CTR(STACK) ((STACK).ctr)

#ifdef DEBUG
#  define POP_DEBUG  POP
#  define PUSH_DEBUG PUSH
#else
#  define POP_DEBUG(STACK)       NOP
#  define PUSH_DEBUG(STACK, VAL) NOP
#endif

/* Ugly macros to simplify the code below. Could probably be C++ lambdas. */

/**
 * Get the next argument from the source specified by 'next_type`.
 */
#define NEXT(VAR, TYPE, MEMBER)                                         \
      do {                                                              \
            switch (next_type) {                                        \
            case OWN_VALIST:                                            \
                  (VAR) = (TYPE)va_arg(args, TYPE);                     \
                  break;                                                \
            case OTHER_VALIST:                                          \
                  assert(ref != NULL);                                  \
                  (VAR) = (TYPE)va_arg(*ref, TYPE);                     \
                  break;                                                \
            case ARG_ARRAY:                                             \
                  assert(a_args);                                       \
                  assert(a_args[a_arg_ctr]);                            \
                  (VAR) = ((a_args[a_arg_ctr][a_arg_subctr]).MEMBER);   \
                  ++a_arg_subctr;                                       \
                  break;                                                \
            default:                                                    \
                  abort();                                              \
            }                                                           \
      } while (0)

/**
 * Get the next argument from the source specified by 'next_type`, but assert
 * that it must not be from an argument array.
 */
#define NEXT_VALIST_ONLY(VAR, TYPE)                 \
      do {                                          \
            switch (next_type) {                    \
            case OWN_VALIST:                        \
                  (VAR) = (TYPE)va_arg(args, TYPE); \
                  break;                            \
            case OTHER_VALIST:                      \
                  assert(ref != NULL);              \
                  (VAR) = (TYPE)va_arg(*ref, TYPE); \
                  break;                            \
            case ARG_ARRAY:                         \
            default:                                \
                  abort();                          \
            }                                       \
      } while (0)

#define ENCODE_FMT_ARRSIZE 128

/*
 * Relatively convenient way to encode an mpack object using a format string.
 * Some of the elements of the string are similar to printf format strings, but
 * otherwise it is entirely custom. All legal values either denote an argument
 * that must be supplied variadically to be encoded in the object, or else to an
 * ignored or special character. Values are not case sensitive.
 *
 * Standard values:
 *     d: Integer value (int)
 *     l: Long integer values (int64_t)
 *     b: Boolean value (int)
 *     s: String (bstring *) - must be a bstring, not a standard C string
 *     c: Char Array (const char *) - NUL terminated C string
 *     n: Nil - no argument expected
 *
 * Any values enclosed in '[' and ']' are placed in a sub-array.
 * Any values enclosed in '{' and '}' are placed in a dictionary, which must
 * have an even number of elements.
 *
 * Three additional special values are recognized:
 *     !: Denotes that the next argument is a pointer to an initialized va_list,
 *        from which all arguments thereafter will be taken, until either another
 *        '!' or '@' is encountered.
 *     @: Denotes an argument of type "mpack_argument **", that is to say an
 *        array of arrays of mpack_argument objects. When '@' is encountered, this
 *        double pointer is taken from the current argument source and used
 *        thereafter as such. The first sub array is used until a '*' is encountered
 *        in the format string, at which point the next array is used, and so on.
 *     *: Increments the current sub array of the mpack_argument ** object.
 *
 * All of the following characters are ignored in the format string, and may be
 * used to make it clearer or more visually pleasing: ':'  ';'  ','  '.', ' '
 *
 * All errors are fatal.
 */
int x;


/**
 * \brief Relatively convenient way to encode an mpack object using a format string.
 * \param size_hint Optional hint at the number of nodes needed to encode.
 * \param fmt The custom format string.
 * \param ... Args.
 * \return Standard string with the (binary) encoded msgpack.
 */
std::string
encode_fmt(unsigned const size_hint, char const *const __restrict fmt, ...)
{
      mpack_argument **a_args;
      unsigned const   arr_size = (size_hint)
                                   ? ENCODE_FMT_ARRSIZE + (size_hint * 6U)
                                   : ENCODE_FMT_ARRSIZE;

#if defined HAVE_CXX_VLA || defined __GNUC__ || defined __clang__
      unsigned sub_lengths[arr_size];
#elif defined HAVE__MALLOCA
      auto *sub_lengths = static_cast<unsigned *>(
          _malloca(sizeof(unsigned) * static_cast<size_t>(arr_size)));
      assert(sub_lengths != nullptr);
#else
      auto *sub_lengths = static_cast<unsigned *>(
          alloca(sizeof(unsigned) * static_cast<size_t>(arr_size)));
#endif

      int         ch;
      va_list     args;
      va_list    *ref       = nullptr;
      char const *ptr       = fmt;
      unsigned   *cur_len   = &sub_lengths[0];
      unsigned    len_ctr   = 1;
      int         next_type = OWN_VALIST;
      *cur_len              = 0;
      a_args                = nullptr;

      NEW_STACK(unsigned *, len_stack);
      va_start(args, fmt);

      /* Go through the format string once to get the number of arguments and
       * in particular the number and size of any arrays. */
      while ((ch = (uchar)*ptr++)) {
            switch (ch) {
            /* Legal values. Increment size and continue. */
            case 'b': case 'B':
            case 'l': case 'L':
            case 'd': case 'D':
            case 's': case 'S':
            case 'n': case 'N':
            case 'c': case 'C':
            case 'u': case 'U':
                  ++*cur_len;
                  break;

            /* New array. Increment current array size, push it onto the
             * stack, and initialize the next counter. */
            case '[':
            case '{':
                  ++*cur_len;
                  PUSH(len_stack, cur_len);
                  cur_len  = &sub_lengths[len_ctr++];
                  *cur_len = 0;
                  break;

            /* End of array. Pop the previous counter off the stack and
             * continue on adding any further elements to it. */
            case ']':
            case '}':
                  cur_len = POP(len_stack);
                  break;

            /* Legal values that do not increment the current size. */
            case ';': case ':': case '.': case ' ':
            case ',': case '!': case '@': case '*':
                  break;

            default:
                  errx("Illegal character \"%c\" found in format.", ch);
            }
      }

      if (STACK_CTR(len_stack) != 0)
            errx("Invalid encode format string: undetermined array/dictionary.\n\"%s\"", fmt);
      if (sub_lengths[0] > 1)
            errx("Invalid encode format string: Cannot encode multiple items "
                 "at the top level. Put them in an array.\n\"%s\"", fmt);
      if (sub_lengths[0] == 0) {
            va_end(args);
            return "";
      }

      std::stringstream ss;
      msgpack::packer   pack{ss};
      // pack.pack_array(sub_lengths[0]);

#if defined HAVE_CXX_VLA || defined __GNUC__ || defined __clang__
      unsigned sub_ctrlist[len_ctr + SIZE_C(1)];
#elif defined HAVE__MALLOCA
      auto *sub_ctrlist = static_cast<unsigned *>(_malloca(sizeof(unsigned) * (len_ctr + SIZE_C(1))));
      assert(sub_ctrlist != nullptr);
#else
      auto *sub_ctrlist = static_cast<unsigned *>(alloca(sizeof(unsigned) * (len_ctr + SIZE_C(1))));
#endif
      unsigned *cur_ctr      = sub_ctrlist;
      unsigned  subctr_ctr   = 1;
      unsigned  a_arg_ctr    = 0;
      unsigned  a_arg_subctr = 0;
      len_ctr                = 1;
      ptr                    = fmt;
      *cur_ctr               = 1;

      RESET(len_stack);
      NEW_STACK(unsigned char, dict_stack);
      PUSH(len_stack, cur_ctr);
      PUSH(dict_stack, 0);

      /* This loop is where all of the actual interpretation and encoding
       * happens. A few stacks are used when recursively encoding arrays and
       * dictionaries to store the state of the enclosing object(s). */
      while ((ch = static_cast<uchar>(*ptr++))) {
            switch (ch) {
            case 'b':
            case 'B': {
                  bool arg = false;
                  NEXT(arg, int, boolean);
                  // mpack_encode_boolean(pack, cur_obj, arg);
                  arg ? pack.pack_true() : pack.pack_false();
            } break;

            case 'd':
            case 'D': {
                  int arg = 0;
                  NEXT(arg, int, num);
                  // mpack_encode_integer(pack, cur_obj, arg);
                  pack.pack_int32(arg);
            } break;

            case 'l':
            case 'L': {
                  int64_t arg = 0;
                  NEXT(arg, int64_t, num);
                  // mpack_encode_integer(pack, cur_obj, arg);
                  pack.pack_int64(arg);
            } break;

            case 'u':
            case 'U': {
                  uint64_t arg = 0;
                  NEXT(arg, uint64_t, uint);
                  // mpack_encode_unsigned(pack, cur_obj, arg);
                  pack.pack_uint64(arg);
            } break;

            case 's':
            case 'S': {
                  std::string *arg = nullptr;
                  NEXT(arg, std::string *, str);
                  // mpack_encode_string(pack, cur_obj, *arg);
                  pack << *arg;
            } break;

            case 'c':
            case 'C': {
                  char const *arg;
                  NEXT(arg, char *, c_str);
                  // mpack_encode_string(pack, cur_obj, arg);
                  pack << arg;
            } break;

            case 'n':
            case 'N':
                  // mpack_encode_nil(pack, cur_obj);
                  pack.pack_nil();
                  break;

            case '[':
                  // mpack_encode_array(pack, cur_obj, sub_lengths[len_ctr]);
                  pack.pack_array(sub_lengths[len_ctr]);
                  PUSH(dict_stack, 0);
                  PUSH(len_stack, cur_ctr);
                  ++len_ctr;
                  cur_ctr  = &sub_ctrlist[subctr_ctr++];
                  *cur_ctr = 0;
                  break;

            case '{':
                  assert((sub_lengths[len_ctr] & 1) == 0);
                  // mpack_encode_dictionary(pack, cur_obj, (sub_lengths[len_ctr] / 2));
                  pack.pack_map(sub_lengths[len_ctr] / 2);
                  PUSH(dict_stack, 1);
                  PUSH(len_stack, cur_ctr);
                  ++len_ctr;
                  cur_ctr  = &sub_ctrlist[subctr_ctr++];
                  *cur_ctr = 0;
                  break;

            case ']':
            case '}':
                  (void)POP(dict_stack);
                  cur_ctr = POP(len_stack);
                  break;

            /* The following use `continue' to skip incrementing the counter */
            case '!':
                  NEXT_VALIST_ONLY(ref, va_list *);
                  next_type = OTHER_VALIST;
                  continue;

            case '@':
                  NEXT_VALIST_ONLY(a_args, mpack_argument **);
                  a_arg_ctr    = 0;
                  a_arg_subctr = 0;
                  assert(a_args[a_arg_ctr]);
                  next_type = ARG_ARRAY;
                  continue;

            case '*':
                  assert(next_type == ARG_ARRAY);
                  ++a_arg_ctr;
                  a_arg_subctr = 0;
                  continue;

            case ';': case ':': case '.': case ' ': case ',':
                  continue;

            default:
                  errx("Somehow (?!) found an invalid character in an mpack format string.");
            }

            ++*cur_ctr;
      }

#ifdef HAVE__MALLOCA
      _freea(sub_lengths);
      _freea(sub_ctrlist);
#endif
      va_end(args);
      return ss.str();
}

} // namespace ipc::mpack
} // namespace emlsp
