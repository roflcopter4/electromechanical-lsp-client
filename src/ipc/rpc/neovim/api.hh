#pragma once
#ifndef HGUARD__IPC__RPC__NEOVIM___API_HH_
#define HGUARD__IPC__RPC__NEOVIM___API_HH_ // NOLINT
/****************************************************************************************/
#include "Common.hh"
#include "ipc/rpc/neovim.hh"

inline namespace emlsp {
namespace ipc::rpc::neovim {
/****************************************************************************************/

namespace demonstration {

namespace detail {
union any_integer {
      uint8_t  u8;
      uint16_t u16;
      uint32_t u32;
      uint64_t u64;
      int8_t   i8;
      int16_t  i16;
      int32_t  i32;
      int64_t  i64;
};
} // namespace detail

using Buffer  = int;
using Window  = int;
using Tabpage = int;
using LuaRef  = void *;

// using Integer = union detail::any_integer;
using Integer = uintmax_t;
using Float   = double;

using Object     = msgpack::object *;
using String     = msgpack::object_str *;
using Dictionary = msgpack::object_map *;
using Array      = msgpack::object_array *;


#define ArrayOf(what)     std::vector<what>
#define ArrayOfN(what, n) std::array<what, n>



ArrayOf(Buffer)      nvim_list_bufs ();
ArrayOf(Dictionary)  nvim_buf_get_keymap (Buffer buffer, String mode);
ArrayOf(Dictionary)  nvim_get_keymap (String mode);
ArrayOf(Integer)     nvim_buf_get_extmark_by_id (Buffer buffer, Integer ns_id, Integer id, Dictionary opts);
ArrayOf(String)      nvim_buf_get_lines (Buffer buffer, Integer start, Integer end, bool strict_indexing);
ArrayOf(String)      nvim_buf_get_text (Buffer buffer, Integer start_row, Integer start_col, Integer end_row, Integer end_col, Dictionary opts);
ArrayOf(String)      nvim_get_runtime_file (String name, bool all);
ArrayOf(String)      nvim_list_runtime_paths ();
ArrayOf(Tabpage)     nvim_list_tabpages ();
ArrayOf(Window)      nvim_list_wins ();
ArrayOf(Window)      nvim_tabpage_list_wins (Tabpage tabpage);
ArrayOfN(Integer, 2) nvim_buf_get_mark (Buffer buffer, String name);
ArrayOfN(Integer, 2) nvim_win_get_cursor (Window window);
ArrayOfN(Integer, 2) nvim_win_get_position (Window window);


Buffer      nvim_create_buf (bool listed, bool scratch);
Buffer      nvim_get_current_buf ();
Buffer      nvim_win_get_buf (Window window);
Tabpage     nvim_get_current_tabpage ();
Tabpage     nvim_win_get_tabpage (Window window);
Window      nvim_get_current_win ();
Window      nvim_open_win (Buffer buffer, bool enter, Dictionary config);
Window      nvim_tabpage_get_win (Tabpage tabpage);
Array       nvim_buf_get_extmarks (Buffer buffer, Integer ns_id, Object start, Object end, Dictionary opts);
Array       nvim_call_atomic (Array calls);
Array       nvim_get_api_info ();
Array       nvim_get_autocmds (Dictionary opts);
Array       nvim_get_mark (String name, Dictionary opts);
Array       nvim_get_proc_children (Integer pid);
Array       nvim_list_chans ();
Array       nvim_list_uis ();
Dictionary  nvim_buf_get_commands (Buffer buffer, Dictionary opts);
Dictionary  nvim_eval_statusline (String str, Dictionary opts);
Dictionary  nvim_get_all_options_info ();
Dictionary  nvim_get_chan_info (Integer chan);
Dictionary  nvim_get_color_map ();
Dictionary  nvim_get_commands (Dictionary opts);
Dictionary  nvim_get_context (Dictionary opts);
Dictionary  nvim_get_hl_by_id (Integer hl_id, bool rgb);
Dictionary  nvim_get_hl_by_name (String name, bool rgb);
Dictionary  nvim_get_mode ();
Dictionary  nvim_get_namespaces ();
Dictionary  nvim_get_option_info (String name);
Dictionary  nvim_parse_expression (String expr, String flags, bool highlight);
Dictionary  nvim_win_get_config (Window window);
Integer     nvim_buf_add_highlight (Buffer buffer, Integer ns_id, String hl_group, Integer line, Integer col_start, Integer col_end);
Integer     nvim_buf_get_changedtick (Buffer buffer);
Integer     nvim_buf_get_offset (Buffer buffer, Integer index);
Integer     nvim_buf_line_count (Buffer buffer);
Integer     nvim_buf_set_extmark (Buffer buffer, Integer ns_id, Integer line, Integer col, Dictionary opts);
Integer     nvim_create_augroup (String name, Dictionary opts);
Integer     nvim_create_autocmd (Object event, Dictionary opts);
Integer     nvim_create_namespace (String name);
Integer     nvim_get_color_by_name (String name);
Integer     nvim_get_hl_id_by_name (String name);
Integer     nvim_input (String keys);
Integer     nvim_open_term (Buffer buffer, Dictionary opts);
Integer     nvim_strwidth (String text);
Integer     nvim_tabpage_get_number (Tabpage tabpage);
Integer     nvim_win_get_height (Window window);
Integer     nvim_win_get_number (Window window);
Integer     nvim_win_get_width (Window window);
Object      nvim_buf_call (Buffer buffer, LuaRef fun);
Object      nvim_buf_get_option (Buffer buffer, String name);
Object      nvim_buf_get_var (Buffer buffer, String name);
Object      nvim_call_dict_function (Object dict, String fn, Array args);
Object      nvim_call_function (String fn, Array args);
Object      nvim_eval (String expr);
Object      nvim_exec_lua (String code, Array args);
Object      nvim_get_option (String name);
Object      nvim_get_option_value (String name, Dictionary opts);
Object      nvim_get_proc (Integer pid);
Object      nvim_get_var (String name);
Object      nvim_get_vvar (String name);
Object      nvim_load_context (Dictionary dict);
Object      nvim_notify (String msg, Integer log_level, Dictionary opts);
Object      nvim_tabpage_get_var (Tabpage tabpage, String name);
Object      nvim_win_call (Window window, LuaRef fun);
Object      nvim_win_get_option (Window window, String name);
Object      nvim_win_get_var (Window window, String name);
String      nvim_buf_get_name (Buffer buffer);
String      nvim_exec (String src, bool output);
String      nvim_get_current_line ();
String      nvim_replace_termcodes (String str, bool from_part, bool do_lt, bool special);
bool        nvim_buf_attach (Buffer buffer, bool send_buffer, Dictionary opts);
bool        nvim_buf_del_extmark (Buffer buffer, Integer ns_id, Integer id);
bool        nvim_buf_del_mark (Buffer buffer, String name);
bool        nvim_buf_detach (Buffer buffer);
bool        nvim_buf_is_loaded (Buffer buffer);
bool        nvim_buf_is_valid (Buffer buffer);
bool        nvim_buf_set_mark (Buffer buffer, String name, Integer line, Integer col, Dictionary opts);
bool        nvim_del_mark (String name);
bool        nvim_paste (String data, bool crlf, Integer phase);
bool        nvim_tabpage_is_valid (Tabpage tabpage);
bool        nvim_win_is_valid (Window window);
void        nvim_add_user_command (String name, Object command, Dictionary opts);
void        nvim_buf_add_user_command (Buffer buffer, String name, Object command, Dictionary opts);
void        nvim_buf_clear_namespace (Buffer buffer, Integer ns_id, Integer line_start, Integer line_end);
void        nvim_buf_del_keymap (Buffer buffer, String mode, String lhs);
void        nvim_buf_del_user_command (Buffer buffer, String name);
void        nvim_buf_del_var (Buffer buffer, String name);
void        nvim_buf_delete (Buffer buffer, Dictionary opts);
void        nvim_buf_set_keymap (Buffer buffer, String mode, String lhs, String rhs, Dictionary opts);
void        nvim_buf_set_lines (Buffer buffer, Integer start, Integer end, bool strict_indexing, ArrayOf(String) replacement);
void        nvim_buf_set_name (Buffer buffer, String name);
void        nvim_buf_set_option (Buffer buffer, String name, Object value);
void        nvim_buf_set_text (Buffer buffer, Integer start_row, Integer start_col, Integer end_row, Integer end_col, ArrayOf(String) replacement);
void        nvim_buf_set_var (Buffer buffer, String name, Object value);
void        nvim_chan_send (Integer chan, String data);
void        nvim_command (String command);
void        nvim_del_augroup_by_id (Integer id);
void        nvim_del_augroup_by_name (String name);
void        nvim_del_autocmd (Integer id);
void        nvim_del_current_line ();
void        nvim_del_keymap (String mode, String lhs);
void        nvim_del_user_command (String name);
void        nvim_del_var (String name);
void        nvim_do_autocmd (Object event, Dictionary opts);
void        nvim_echo (Array chunks, bool history, Dictionary opts);
void        nvim_err_write (String str);
void        nvim_err_writeln (String str);
void        nvim_feedkeys (String keys, String mode, bool escape_ks);
void        nvim_input_mouse (String button, String action, String modifier, Integer grid, Integer row, Integer col);
void        nvim_out_write (String str);
void        nvim_put (ArrayOf(String) lines, String type, bool after, bool follow);
void        nvim_select_popupmenu_item (Integer item, bool insert, bool finish, Dictionary opts);
void        nvim_set_client_info (String name, Dictionary version, String type, Dictionary methods, Dictionary attributes);
void        nvim_set_current_buf (Buffer buffer);
void        nvim_set_current_dir (String dir);
void        nvim_set_current_line (String line);
void        nvim_set_current_tabpage (Tabpage tabpage);
void        nvim_set_current_win (Window window);
void        nvim_set_decoration_provider (Integer ns_id, Dictionary opts);
void        nvim_set_hl (Integer ns_id, String name, Dictionary val);
void        nvim_set_keymap (String mode, String lhs, String rhs, Dictionary opts);
void        nvim_set_option (String name, Object value);
void        nvim_set_option_value (String name, Object value, Dictionary opts);
void        nvim_set_var (String name, Object value);
void        nvim_set_vvar (String name, Object value);
void        nvim_subscribe (String event);
void        nvim_tabpage_del_var (Tabpage tabpage, String name);
void        nvim_tabpage_set_var (Tabpage tabpage, String name, Object value);
void        nvim_ui_attach (Integer width, Integer height, Dictionary options);
void        nvim_ui_detach ();
void        nvim_ui_pum_set_bounds (Float width, Float height, Float row, Float col);
void        nvim_ui_pum_set_height (Integer height);
void        nvim_ui_set_option (String name, Object value);
void        nvim_ui_try_resize (Integer width, Integer height);
void        nvim_ui_try_resize_grid (Integer grid, Integer width, Integer height);
void        nvim_unsubscribe (String event);
void        nvim_win_close (Window window, bool force);
void        nvim_win_del_var (Window window, String name);
void        nvim_win_hide (Window window);
void        nvim_win_set_buf (Window window, Buffer buffer);
void        nvim_win_set_config (Window window, Dictionary config);
void        nvim_win_set_cursor (Window window, ArrayOfN(Integer, 2) pos);
void        nvim_win_set_height (Window window, Integer height);
void        nvim_win_set_option (Window window, String name, Object value);
void        nvim_win_set_var (Window window, String name, Object value);
void        nvim_win_set_width (Window window, Integer width);



using nvim_error_event           = std::pair<int /*type*/, String /*message*/>;
using nvim_buf_lines_event       = std::tuple<Buffer, uint64_t /*changedtick*/, int64_t /*firstline*/, int64_t /*lastline*/, ArrayOf(String) /*linedata*/, bool /*more*/>;
using nvim_buf_changedtick_event = std::pair<Buffer, uint64_t /*changedtick*/>;
using nvim_buf_detach_event      = Buffer;

} // namespace demonstration

/* 
 *  Neovim response messages:
 *  [int, int, Object, Object]
 *  
 *  1) Type of message (request, notification, response)
 *  2) Message ID
 *  3) If an error occured: Object with error details (generally a string)
 *     Else: Nil
 *  4) Normally: Object with the message contents
 *     On error: Nil
 *
 *
 *  Neovim notifications:
 *  [int, String, Object]
 *
 *  1) Type of message (request, notification, response)
 *  2) Name of notification
 *  3) Data
 *
 *
 *  Request:
 *  [int, String, Object]
 *  Basically same as notification.
 *
 *
 *  request      = 0
 *  reply        = 1
 *  notification = 2
 */


/*======================================================================================*/


class nvim_message_handler
{

};


/*======================================================================================*/



#if 0

void                    nvim_add_user_command (String name, Object command, Dictionary opts);
Integer                 nvim_buf_add_highlight (Buffer buffer, Integer ns_id, String hl_group, Integer line, Integer col_start, Integer col_end);
void                    nvim_buf_add_user_command (Buffer buffer, String name, Object command, Dictionary opts);
bool                    nvim_buf_attach (Buffer buffer, bool send_buffer, Dictionary opts);
Object                  nvim_buf_call (Buffer buffer, LuaRef fun);
void                    nvim_buf_clear_namespace (Buffer buffer, Integer ns_id, Integer line_start, Integer line_end);
bool                    nvim_buf_del_extmark (Buffer buffer, Integer ns_id, Integer id);
void                    nvim_buf_del_keymap (Buffer buffer, String mode, String lhs);
bool                    nvim_buf_del_mark (Buffer buffer, String name);
void                    nvim_buf_del_user_command (Buffer buffer, String name);
void                    nvim_buf_del_var (Buffer buffer, String name);
void                    nvim_buf_delete (Buffer buffer, Dictionary opts);
bool                    nvim_buf_detach (Buffer buffer);
Integer                 nvim_buf_get_changedtick (Buffer buffer);
Dictionary              nvim_buf_get_commands (Buffer buffer, Dictionary opts);
std::vector<Integer>    nvim_buf_get_extmark_by_id(Buffer buffer, Integer ns_id, Integer id, Dictionary opts);
Array                   nvim_buf_get_extmarks (Buffer buffer, Integer ns_id, Object start, Object end, Dictionary opts);
std::vector<Dictionary> nvim_buf_get_keymap(Buffer buffer, String mode);
std::vector<String>     nvim_buf_get_lines(Buffer buffer, Integer start, Integer end, bool strict_indexing);
std::array<Integer, 2>  nvim_buf_get_mark(Buffer buffer, String name);
String                  nvim_buf_get_name (Buffer buffer);
Integer                 nvim_buf_get_offset (Buffer buffer, Integer index);
Object                  nvim_buf_get_option (Buffer buffer, String name);
std::vector<String>     nvim_buf_get_text(Buffer buffer, Integer start_row, Integer start_col, Integer end_row, Integer end_col, Dictionary opts);
Object                  nvim_buf_get_var (Buffer buffer, String name);
bool                    nvim_buf_is_loaded (Buffer buffer);
bool                    nvim_buf_is_valid (Buffer buffer);
Integer                 nvim_buf_line_count (Buffer buffer);
Integer                 nvim_buf_set_extmark (Buffer buffer, Integer ns_id, Integer line, Integer col, Dictionary opts);
void                    nvim_buf_set_keymap (Buffer buffer, String mode, String lhs, String rhs, Dictionary opts);
void                    nvim_buf_set_lines(Buffer buffer, Integer start, Integer end, bool strict_indexing, std::vector<String> replacement);
bool                    nvim_buf_set_mark (Buffer buffer, String name, Integer line, Integer col, Dictionary opts);
void                    nvim_buf_set_name (Buffer buffer, String name);
void                    nvim_buf_set_option (Buffer buffer, String name, Object value);
void                    nvim_buf_set_text(Buffer buffer, Integer start_row, Integer start_col, Integer end_row, Integer end_col, std::vector<String> replacement);
void                    nvim_buf_set_var (Buffer buffer, String name, Object value);
Array                   nvim_call_atomic (Array calls);
Object                  nvim_call_dict_function (Object dict, String fn, Array args);
Object                  nvim_call_function (String fn, Array args);
void                    nvim_chan_send (Integer chan, String data);
void                    nvim_command (String command);
Integer                 nvim_create_augroup (String name, Dictionary opts);
Integer                 nvim_create_autocmd (Object event, Dictionary opts);
Buffer                  nvim_create_buf (bool listed, bool scratch);
Integer                 nvim_create_namespace (String name);
void                    nvim_del_augroup_by_id (Integer id);
void                    nvim_del_augroup_by_name (String name);
void                    nvim_del_autocmd (Integer id);
void                    nvim_del_current_line ();
void                    nvim_del_keymap (String mode, String lhs);
bool                    nvim_del_mark (String name);
void                    nvim_del_user_command (String name);
void                    nvim_del_var (String name);
void                    nvim_do_autocmd (Object event, Dictionary opts);
void                    nvim_echo (Array chunks, bool history, Dictionary opts);
void                    nvim_err_write (String str);
void                    nvim_err_writeln (String str);
Object                  nvim_eval (String expr);
Dictionary              nvim_eval_statusline (String str, Dictionary opts);
String                  nvim_exec (String src, bool output);
Object                  nvim_exec_lua (String code, Array args);
void                    nvim_feedkeys (String keys, String mode, bool escape_ks);
Dictionary              nvim_get_all_options_info ();
Array                   nvim_get_api_info ();
Array                   nvim_get_autocmds (Dictionary opts);
Dictionary              nvim_get_chan_info (Integer chan);
Integer                 nvim_get_color_by_name (String name);
Dictionary              nvim_get_color_map ();
Dictionary              nvim_get_commands (Dictionary opts);
Dictionary              nvim_get_context (Dictionary opts);
Buffer                  nvim_get_current_buf ();
String                  nvim_get_current_line ();
Tabpage                 nvim_get_current_tabpage ();
Window                  nvim_get_current_win ();
Dictionary              nvim_get_hl_by_id (Integer hl_id, bool rgb);
Dictionary              nvim_get_hl_by_name (String name, bool rgb);
Integer                 nvim_get_hl_id_by_name (String name);
std::vector<Dictionary> nvim_get_keymap(String mode);
Array                   nvim_get_mark (String name, Dictionary opts);
Dictionary              nvim_get_mode ();
Dictionary              nvim_get_namespaces ();
Object                  nvim_get_option (String name);
Dictionary              nvim_get_option_info (String name);
Object                  nvim_get_option_value (String name, Dictionary opts);
Object                  nvim_get_proc (Integer pid);
Array                   nvim_get_proc_children (Integer pid);
std::vector<String>     nvim_get_runtime_file(String name, bool all);
Object                  nvim_get_var (String name);
Object                  nvim_get_vvar (String name);
Integer                 nvim_input (String keys);
void                    nvim_input_mouse (String button, String action, String modifier, Integer grid, Integer row, Integer col);
std::vector<Buffer>     nvim_list_bufs();
Array                   nvim_list_chans ();
std::vector<String>     nvim_list_runtime_paths();
std::vector<Tabpage>    nvim_list_tabpages();
Array                   nvim_list_uis ();
std::vector<Window>     nvim_list_wins();
Object                  nvim_load_context (Dictionary dict);
Object                  nvim_notify (String msg, Integer log_level, Dictionary opts);
Integer                 nvim_open_term (Buffer buffer, Dictionary opts);
Window                  nvim_open_win (Buffer buffer, bool enter, Dictionary config);
void                    nvim_out_write (String str);
Dictionary              nvim_parse_expression (String expr, String flags, bool highlight);
bool                    nvim_paste (String data, bool crlf, Integer phase);
void                    nvim_put(std::vector<String> lines, String type, bool after, bool follow);
String                  nvim_replace_termcodes (String str, bool from_part, bool do_lt, bool special);
void                    nvim_select_popupmenu_item (Integer item, bool insert, bool finish, Dictionary opts);
void                    nvim_set_client_info (String name, Dictionary version, String type, Dictionary methods, Dictionary attributes);
void                    nvim_set_current_buf (Buffer buffer);
void                    nvim_set_current_dir (String dir);
void                    nvim_set_current_line (String line);
void                    nvim_set_current_tabpage (Tabpage tabpage);
void                    nvim_set_current_win (Window window);
void                    nvim_set_decoration_provider (Integer ns_id, Dictionary opts);
void                    nvim_set_hl (Integer ns_id, String name, Dictionary val);
void                    nvim_set_keymap (String mode, String lhs, String rhs, Dictionary opts);
void                    nvim_set_option (String name, Object value);
void                    nvim_set_option_value (String name, Object value, Dictionary opts);
void                    nvim_set_var (String name, Object value);
void                    nvim_set_vvar (String name, Object value);
Integer                 nvim_strwidth (String text);
void                    nvim_subscribe (String event);
void                    nvim_tabpage_del_var (Tabpage tabpage, String name);
Integer                 nvim_tabpage_get_number (Tabpage tabpage);
Object                  nvim_tabpage_get_var (Tabpage tabpage, String name);
Window                  nvim_tabpage_get_win (Tabpage tabpage);
bool                    nvim_tabpage_is_valid (Tabpage tabpage);
std::vector<Window>     nvim_tabpage_list_wins(Tabpage tabpage);
void                    nvim_tabpage_set_var (Tabpage tabpage, String name, Object value);
void                    nvim_ui_attach (Integer width, Integer height, Dictionary options);
void                    nvim_ui_detach ();
void                    nvim_ui_pum_set_bounds (Float width, Float height, Float row, Float col);
void                    nvim_ui_pum_set_height (Integer height);
void                    nvim_ui_set_option (String name, Object value);
void                    nvim_ui_try_resize (Integer width, Integer height);
void                    nvim_ui_try_resize_grid (Integer grid, Integer width, Integer height);
void                    nvim_unsubscribe (String event);
Object                  nvim_win_call (Window window, LuaRef fun);
void                    nvim_win_close (Window window, bool force);
void                    nvim_win_del_var (Window window, String name);
Buffer                  nvim_win_get_buf (Window window);
Dictionary              nvim_win_get_config (Window window);
std::array<Integer, 2>  nvim_win_get_cursor(Window window);
Integer                 nvim_win_get_height (Window window);
Integer                 nvim_win_get_number (Window window);
Object                  nvim_win_get_option (Window window, String name);
std::array<Integer, 2>  nvim_win_get_position(Window window);
Tabpage                 nvim_win_get_tabpage (Window window);
Object                  nvim_win_get_var (Window window, String name);
Integer                 nvim_win_get_width (Window window);
void                    nvim_win_hide (Window window);
bool                    nvim_win_is_valid (Window window);
void                    nvim_win_set_buf (Window window, Buffer buffer);
void                    nvim_win_set_config (Window window, Dictionary config);
void                    nvim_win_set_cursor(Window window, std::array<Integer, 2> pos);
void                    nvim_win_set_height (Window window, Integer height);
void                    nvim_win_set_option (Window window, String name, Object value);
void                    nvim_win_set_var (Window window, String name, Object value);
void                    nvim_win_set_width (Window window, Integer width);

#endif


/****************************************************************************************/
} // namespace ipc::rpc::neovim
} // namespace emlsp
#endif
