#pragma once
#ifndef HGUARD__IPC__RPC__NEOVIM___API_HH_
#define HGUARD__IPC__RPC__NEOVIM___API_HH_ // NOLINT
/****************************************************************************************/
#include "Common.hh"
#include "ipc/rpc/neovim.hh"

inline namespace MAIN_PACKAGE_NAMESPACE {
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


#define ArrayOf(what)     std::vector<what>    //NOLINT(*-macro-usage)
#define ArrayOfN(what, n) std::array<what, n>  //NOLINT(*-macro-usage)


extern ArrayOf(Buffer)     nvim_list_bufs ();
extern ArrayOf(Dictionary) nvim_buf_get_keymap (Buffer buffer, String mode);
extern ArrayOf(Dictionary) nvim_get_keymap (String mode);
extern ArrayOf(Integer)    nvim_buf_get_extmark_by_id (Buffer buffer, Integer ns_id, Integer id, Dictionary opts);
extern ArrayOf(String)     nvim_buf_get_lines (Buffer buffer, Integer start, Integer end, bool strict_indexing);
extern ArrayOf(String)     nvim_buf_get_text (Buffer buffer, Integer start_row, Integer start_col, Integer end_row, Integer end_col, Dictionary opts);
extern ArrayOf(String)     nvim_get_runtime_file (String name, bool all);
extern ArrayOf(String)     nvim_list_runtime_paths ();
extern ArrayOf(Tabpage)    nvim_list_tabpages ();
extern ArrayOf(Window)     nvim_list_wins ();
extern ArrayOf(Window)     nvim_tabpage_list_wins (Tabpage tabpage);
extern ArrayOfN(Integer,2) nvim_buf_get_mark (Buffer buffer, String name);
extern ArrayOfN(Integer,2) nvim_win_get_cursor (Window window);
extern ArrayOfN(Integer,2) nvim_win_get_position (Window window);


extern Buffer      nvim_create_buf (bool listed, bool scratch);
extern Buffer      nvim_get_current_buf ();
extern Buffer      nvim_win_get_buf (Window window);
extern Tabpage     nvim_get_current_tabpage ();
extern Tabpage     nvim_win_get_tabpage (Window window);
extern Window      nvim_get_current_win ();
extern Window      nvim_open_win (Buffer buffer, bool enter, Dictionary config);
extern Window      nvim_tabpage_get_win (Tabpage tabpage);
extern Array       nvim_buf_get_extmarks (Buffer buffer, Integer ns_id, Object start, Object end, Dictionary opts);
extern Array       nvim_call_atomic (Array calls);
extern Array       nvim_get_api_info ();
extern Array       nvim_get_autocmds (Dictionary opts);
extern Array       nvim_get_mark (String name, Dictionary opts);
extern Array       nvim_get_proc_children (Integer pid);
extern Array       nvim_list_chans ();
extern Array       nvim_list_uis ();
extern Dictionary  nvim_buf_get_commands (Buffer buffer, Dictionary opts);
extern Dictionary  nvim_eval_statusline (String str, Dictionary opts);
extern Dictionary  nvim_get_all_options_info ();
extern Dictionary  nvim_get_chan_info (Integer chan);
extern Dictionary  nvim_get_color_map ();
extern Dictionary  nvim_get_commands (Dictionary opts);
extern Dictionary  nvim_get_context (Dictionary opts);
extern Dictionary  nvim_get_hl_by_id (Integer hl_id, bool rgb);
extern Dictionary  nvim_get_hl_by_name (String name, bool rgb);
extern Dictionary  nvim_get_mode ();
extern Dictionary  nvim_get_namespaces ();
extern Dictionary  nvim_get_option_info (String name);
extern Dictionary  nvim_parse_expression (String expr, String flags, bool highlight);
extern Dictionary  nvim_win_get_config (Window window);
extern Integer     nvim_buf_add_highlight (Buffer buffer, Integer ns_id, String hl_group, Integer line, Integer col_start, Integer col_end);
extern Integer     nvim_buf_get_changedtick (Buffer buffer);
extern Integer     nvim_buf_get_offset (Buffer buffer, Integer index);
extern Integer     nvim_buf_line_count (Buffer buffer);
extern Integer     nvim_buf_set_extmark (Buffer buffer, Integer ns_id, Integer line, Integer col, Dictionary opts);
extern Integer     nvim_create_augroup (String name, Dictionary opts);
extern Integer     nvim_create_autocmd (Object event, Dictionary opts);
extern Integer     nvim_create_namespace (String name);
extern Integer     nvim_get_color_by_name (String name);
extern Integer     nvim_get_hl_id_by_name (String name);
extern Integer     nvim_input (String keys);
extern Integer     nvim_open_term (Buffer buffer, Dictionary opts);
extern Integer     nvim_strwidth (String text);
extern Integer     nvim_tabpage_get_number (Tabpage tabpage);
extern Integer     nvim_win_get_height (Window window);
extern Integer     nvim_win_get_number (Window window);
extern Integer     nvim_win_get_width (Window window);
extern Object      nvim_buf_call (Buffer buffer, LuaRef fun);
extern Object      nvim_buf_get_option (Buffer buffer, String name);
extern Object      nvim_buf_get_var (Buffer buffer, String name);
extern Object      nvim_call_dict_function (Object dict, String fn, Array args);
extern Object      nvim_call_function (String fn, Array args);
extern Object      nvim_eval (String expr);
extern Object      nvim_exec_lua (String code, Array args);
extern Object      nvim_get_option (String name);
extern Object      nvim_get_option_value (String name, Dictionary opts);
extern Object      nvim_get_proc (Integer pid);
extern Object      nvim_get_var (String name);
extern Object      nvim_get_vvar (String name);
extern Object      nvim_load_context (Dictionary dict);
extern Object      nvim_notify (String msg, Integer log_level, Dictionary opts);
extern Object      nvim_tabpage_get_var (Tabpage tabpage, String name);
extern Object      nvim_win_call (Window window, LuaRef fun);
extern Object      nvim_win_get_option (Window window, String name);
extern Object      nvim_win_get_var (Window window, String name);
extern String      nvim_buf_get_name (Buffer buffer);
extern String      nvim_exec (String src, bool output);
extern String      nvim_get_current_line ();
extern String      nvim_replace_termcodes (String str, bool from_part, bool do_lt, bool special);
extern bool        nvim_buf_attach (Buffer buffer, bool send_buffer, Dictionary opts);
extern bool        nvim_buf_del_extmark (Buffer buffer, Integer ns_id, Integer id);
extern bool        nvim_buf_del_mark (Buffer buffer, String name);
extern bool        nvim_buf_detach (Buffer buffer);
extern bool        nvim_buf_is_loaded (Buffer buffer);
extern bool        nvim_buf_is_valid (Buffer buffer);
extern bool        nvim_buf_set_mark (Buffer buffer, String name, Integer line, Integer col, Dictionary opts);
extern bool        nvim_del_mark (String name);
extern bool        nvim_paste (String data, bool crlf, Integer phase);
extern bool        nvim_tabpage_is_valid (Tabpage tabpage);
extern bool        nvim_win_is_valid (Window window);
extern void        nvim_add_user_command (String name, Object command, Dictionary opts);
extern void        nvim_buf_add_user_command (Buffer buffer, String name, Object command, Dictionary opts);
extern void        nvim_buf_clear_namespace (Buffer buffer, Integer ns_id, Integer line_start, Integer line_end);
extern void        nvim_buf_del_keymap (Buffer buffer, String mode, String lhs);
extern void        nvim_buf_del_user_command (Buffer buffer, String name);
extern void        nvim_buf_del_var (Buffer buffer, String name);
extern void        nvim_buf_delete (Buffer buffer, Dictionary opts);
extern void        nvim_buf_set_keymap (Buffer buffer, String mode, String lhs, String rhs, Dictionary opts);
extern void        nvim_buf_set_lines (Buffer buffer, Integer start, Integer end, bool strict_indexing, ArrayOf(String) replacement);
extern void        nvim_buf_set_name (Buffer buffer, String name);
extern void        nvim_buf_set_option (Buffer buffer, String name, Object value);
extern void        nvim_buf_set_text (Buffer buffer, Integer start_row, Integer start_col, Integer end_row, Integer end_col, ArrayOf(String) replacement);
extern void        nvim_buf_set_var (Buffer buffer, String name, Object value);
extern void        nvim_chan_send (Integer chan, String data);
extern void        nvim_command (String command);
extern void        nvim_del_augroup_by_id (Integer id);
extern void        nvim_del_augroup_by_name (String name);
extern void        nvim_del_autocmd (Integer id);
extern void        nvim_del_current_line ();
extern void        nvim_del_keymap (String mode, String lhs);
extern void        nvim_del_user_command (String name);
extern void        nvim_del_var (String name);
extern void        nvim_do_autocmd (Object event, Dictionary opts);
extern void        nvim_echo (Array chunks, bool history, Dictionary opts);
extern void        nvim_err_write (String str);
extern void        nvim_err_writeln (String str);
extern void        nvim_feedkeys (String keys, String mode, bool escape_ks);
extern void        nvim_input_mouse (String button, String action, String modifier, Integer grid, Integer row, Integer col);
extern void        nvim_out_write (String str);
extern void        nvim_put (ArrayOf(String) lines, String type, bool after, bool follow);
extern void        nvim_select_popupmenu_item (Integer item, bool insert, bool finish, Dictionary opts);
extern void        nvim_set_client_info (String name, Dictionary version, String type, Dictionary methods, Dictionary attributes);
extern void        nvim_set_current_buf (Buffer buffer);
extern void        nvim_set_current_dir (String dir);
extern void        nvim_set_current_line (String line);
extern void        nvim_set_current_tabpage (Tabpage tabpage);
extern void        nvim_set_current_win (Window window);
extern void        nvim_set_decoration_provider (Integer ns_id, Dictionary opts);
extern void        nvim_set_hl (Integer ns_id, String name, Dictionary val);
extern void        nvim_set_keymap (String mode, String lhs, String rhs, Dictionary opts);
extern void        nvim_set_option (String name, Object value);
extern void        nvim_set_option_value (String name, Object value, Dictionary opts);
extern void        nvim_set_var (String name, Object value);
extern void        nvim_set_vvar (String name, Object value);
extern void        nvim_subscribe (String event);
extern void        nvim_tabpage_del_var (Tabpage tabpage, String name);
extern void        nvim_tabpage_set_var (Tabpage tabpage, String name, Object value);
extern void        nvim_ui_attach (Integer width, Integer height, Dictionary options);
extern void        nvim_ui_detach ();
extern void        nvim_ui_pum_set_bounds (Float width, Float height, Float row, Float col);
extern void        nvim_ui_pum_set_height (Integer height);
extern void        nvim_ui_set_option (String name, Object value);
extern void        nvim_ui_try_resize (Integer width, Integer height);
extern void        nvim_ui_try_resize_grid (Integer grid, Integer width, Integer height);
extern void        nvim_unsubscribe (String event);
extern void        nvim_win_close (Window window, bool force);
extern void        nvim_win_del_var (Window window, String name);
extern void        nvim_win_hide (Window window);
extern void        nvim_win_set_buf (Window window, Buffer buffer);
extern void        nvim_win_set_config (Window window, Dictionary config);
extern void        nvim_win_set_cursor (Window window, ArrayOfN(Integer, 2) pos);
extern void        nvim_win_set_height (Window window, Integer height);
extern void        nvim_win_set_option (Window window, String name, Object value);
extern void        nvim_win_set_var (Window window, String name, Object value);
extern void        nvim_win_set_width (Window window, Integer width);


constexpr char const *const neovim_api_methods[] = {
      "nvim_list_bufs",
      "nvim_buf_get_keymap",
      "nvim_get_keymap",
      "nvim_buf_get_extmark_by_id",
      "nvim_buf_get_lines",
      "nvim_buf_get_text",
      "nvim_get_runtime_file",
      "nvim_list_runtime_paths",
      "nvim_list_tabpages",
      "nvim_list_wins",
      "nvim_tabpage_list_wins",
      "nvim_buf_get_mark",
      "nvim_win_get_cursor",
      "nvim_win_get_position",

      "nvim_create_buf",
      "nvim_get_current_buf",
      "nvim_win_get_buf",
      "nvim_get_current_tabpage",
      "nvim_win_get_tabpage",
      "nvim_get_current_win",
      "nvim_open_win",
      "nvim_tabpage_get_win",
      "nvim_buf_get_extmarks",
      "nvim_call_atomic",
      "nvim_get_api_info",
      "nvim_get_autocmds",
      "nvim_get_mark",
      "nvim_get_proc_children",
      "nvim_list_chans",
      "nvim_list_uis",
      "nvim_buf_get_commands",
      "nvim_eval_statusline",
      "nvim_get_all_options_info",
      "nvim_get_chan_info",
      "nvim_get_color_map",
      "nvim_get_commands",
      "nvim_get_context",
      "nvim_get_hl_by_id",
      "nvim_get_hl_by_name",
      "nvim_get_mode",
      "nvim_get_namespaces",
      "nvim_get_option_info",
      "nvim_parse_expression",
      "nvim_win_get_config",
      "nvim_buf_add_highlight",
      "nvim_buf_get_changedtick",
      "nvim_buf_get_offset",
      "nvim_buf_line_count",
      "nvim_buf_set_extmark",
      "nvim_create_augroup",
      "nvim_create_autocmd",
      "nvim_create_namespace",
      "nvim_get_color_by_name",
      "nvim_get_hl_id_by_name",
      "nvim_input",
      "nvim_open_term",
      "nvim_strwidth",
      "nvim_tabpage_get_number",
      "nvim_win_get_height",
      "nvim_win_get_number",
      "nvim_win_get_width",
      "nvim_buf_call",
      "nvim_buf_get_option",
      "nvim_buf_get_var",
      "nvim_call_dict_function",
      "nvim_call_function",
      "nvim_eval",
      "nvim_exec_lua",
      "nvim_get_option",
      "nvim_get_option_value",
      "nvim_get_proc",
      "nvim_get_var",
      "nvim_get_vvar",
      "nvim_load_context",
      "nvim_notify",
      "nvim_tabpage_get_var",
      "nvim_win_call",
      "nvim_win_get_option",
      "nvim_win_get_var",
      "nvim_buf_get_name",
      "nvim_exec",
      "nvim_get_current_line",
      "nvim_replace_termcodes",
      "nvim_buf_attach",
      "nvim_buf_del_extmark",
      "nvim_buf_del_mark",
      "nvim_buf_detach",
      "nvim_buf_is_loaded",
      "nvim_buf_is_valid",
      "nvim_buf_set_mark",
      "nvim_del_mark",
      "nvim_paste",
      "nvim_tabpage_is_valid",
      "nvim_win_is_valid",
      "nvim_add_user_command",
      "nvim_buf_add_user_command",
      "nvim_buf_clear_namespace",
      "nvim_buf_del_keymap",
      "nvim_buf_del_user_command",
      "nvim_buf_del_var",
      "nvim_buf_delete",
      "nvim_buf_set_keymap",
      "nvim_buf_set_lines",
      "nvim_buf_set_name",
      "nvim_buf_set_option",
      "nvim_buf_set_text",
      "nvim_buf_set_var",
      "nvim_chan_send",
      "nvim_command",
      "nvim_del_augroup_by_id",
      "nvim_del_augroup_by_name",
      "nvim_del_autocmd",
      "nvim_del_current_line",
      "nvim_del_keymap",
      "nvim_del_user_command",
      "nvim_del_var",
      "nvim_do_autocmd",
      "nvim_echo",
      "nvim_err_write",
      "nvim_err_writeln",
      "nvim_feedkeys",
      "nvim_input_mouse",
      "nvim_out_write",
      "nvim_put",
      "nvim_select_popupmenu_item",
      "nvim_set_client_info",
      "nvim_set_current_buf",
      "nvim_set_current_dir",
      "nvim_set_current_line",
      "nvim_set_current_tabpage",
      "nvim_set_current_win",
      "nvim_set_decoration_provider",
      "nvim_set_hl",
      "nvim_set_keymap",
      "nvim_set_option",
      "nvim_set_option_value",
      "nvim_set_var",
      "nvim_set_vvar",
      "nvim_subscribe",
      "nvim_tabpage_del_var",
      "nvim_tabpage_set_var",
      "nvim_ui_attach",
      "nvim_ui_detach",
      "nvim_ui_pum_set_bounds",
      "nvim_ui_pum_set_height",
      "nvim_ui_set_option",
      "nvim_ui_try_resize",
      "nvim_ui_try_resize_grid",
      "nvim_unsubscribe",
      "nvim_win_close",
      "nvim_win_del_var",
      "nvim_win_hide",
      "nvim_win_set_buf",
      "nvim_win_set_config",
      "nvim_win_set_cursor",
      "nvim_win_set_height",
      "nvim_win_set_option",
      "nvim_win_set_var",
      "nvim_win_set_width",
};



constexpr char const *const neovim_api_methods_sorted[] = {
      "nvim_add_user_command",
      "nvim_buf_add_highlight",
      "nvim_buf_add_user_command",
      "nvim_buf_attach",
      "nvim_buf_call",
      "nvim_buf_clear_namespace",
      "nvim_buf_del_extmark",
      "nvim_buf_del_keymap",
      "nvim_buf_del_mark",
      "nvim_buf_del_user_command",
      "nvim_buf_del_var",
      "nvim_buf_delete",
      "nvim_buf_detach",
      "nvim_buf_get_changedtick",
      "nvim_buf_get_commands",
      "nvim_buf_get_extmark_by_id",
      "nvim_buf_get_extmarks",
      "nvim_buf_get_keymap",
      "nvim_buf_get_lines",
      "nvim_buf_get_mark",
      "nvim_buf_get_name",
      "nvim_buf_get_offset",
      "nvim_buf_get_option",
      "nvim_buf_get_text",
      "nvim_buf_get_var",
      "nvim_buf_is_loaded",
      "nvim_buf_is_valid",
      "nvim_buf_line_count",
      "nvim_buf_set_extmark",
      "nvim_buf_set_keymap",
      "nvim_buf_set_lines",
      "nvim_buf_set_mark",
      "nvim_buf_set_name",
      "nvim_buf_set_option",
      "nvim_buf_set_text",
      "nvim_buf_set_var",
      "nvim_call_atomic",
      "nvim_call_dict_function",
      "nvim_call_function",
      "nvim_chan_send",
      "nvim_command",
      "nvim_create_augroup",
      "nvim_create_autocmd",
      "nvim_create_buf",
      "nvim_create_namespace",
      "nvim_del_augroup_by_id",
      "nvim_del_augroup_by_name",
      "nvim_del_autocmd",
      "nvim_del_current_line",
      "nvim_del_keymap",
      "nvim_del_mark",
      "nvim_del_user_command",
      "nvim_del_var",
      "nvim_do_autocmd",
      "nvim_echo",
      "nvim_err_write",
      "nvim_err_writeln",
      "nvim_eval",
      "nvim_eval_statusline",
      "nvim_exec",
      "nvim_exec_lua",
      "nvim_feedkeys",
      "nvim_get_all_options_info",
      "nvim_get_api_info",
      "nvim_get_autocmds",
      "nvim_get_chan_info",
      "nvim_get_color_by_name",
      "nvim_get_color_map",
      "nvim_get_commands",
      "nvim_get_context",
      "nvim_get_current_buf",
      "nvim_get_current_line",
      "nvim_get_current_tabpage",
      "nvim_get_current_win",
      "nvim_get_hl_by_id",
      "nvim_get_hl_by_name",
      "nvim_get_hl_id_by_name",
      "nvim_get_keymap",
      "nvim_get_mark",
      "nvim_get_mode",
      "nvim_get_namespaces",
      "nvim_get_option",
      "nvim_get_option_info",
      "nvim_get_option_value",
      "nvim_get_proc",
      "nvim_get_proc_children",
      "nvim_get_runtime_file",
      "nvim_get_var",
      "nvim_get_vvar",
      "nvim_input",
      "nvim_input_mouse",
      "nvim_list_bufs",
      "nvim_list_chans",
      "nvim_list_runtime_paths",
      "nvim_list_tabpages",
      "nvim_list_uis",
      "nvim_list_wins",
      "nvim_load_context",
      "nvim_notify",
      "nvim_open_term",
      "nvim_open_win",
      "nvim_out_write",
      "nvim_parse_expression",
      "nvim_paste",
      "nvim_put",
      "nvim_replace_termcodes",
      "nvim_select_popupmenu_item",
      "nvim_set_client_info",
      "nvim_set_current_buf",
      "nvim_set_current_dir",
      "nvim_set_current_line",
      "nvim_set_current_tabpage",
      "nvim_set_current_win",
      "nvim_set_decoration_provider",
      "nvim_set_hl",
      "nvim_set_keymap",
      "nvim_set_option",
      "nvim_set_option_value",
      "nvim_set_var",
      "nvim_set_vvar",
      "nvim_strwidth",
      "nvim_subscribe",
      "nvim_tabpage_del_var",
      "nvim_tabpage_get_number",
      "nvim_tabpage_get_var",
      "nvim_tabpage_get_win",
      "nvim_tabpage_is_valid",
      "nvim_tabpage_list_wins",
      "nvim_tabpage_set_var",
      "nvim_ui_attach",
      "nvim_ui_detach",
      "nvim_ui_pum_set_bounds",
      "nvim_ui_pum_set_height",
      "nvim_ui_set_option",
      "nvim_ui_try_resize",
      "nvim_ui_try_resize_grid",
      "nvim_unsubscribe",
      "nvim_win_call",
      "nvim_win_close",
      "nvim_win_del_var",
      "nvim_win_get_buf",
      "nvim_win_get_config",
      "nvim_win_get_cursor",
      "nvim_win_get_height",
      "nvim_win_get_number",
      "nvim_win_get_option",
      "nvim_win_get_position",
      "nvim_win_get_tabpage",
      "nvim_win_get_var",
      "nvim_win_get_width",
      "nvim_win_hide",
      "nvim_win_is_valid",
      "nvim_win_set_buf",
      "nvim_win_set_config",
      "nvim_win_set_cursor",
      "nvim_win_set_height",
      "nvim_win_set_option",
      "nvim_win_set_var",
      "nvim_win_set_width",
};


using nvim_error_event           = std::pair<int /*type*/, String /*message*/>;
//using nvim_buf_lines_event       = std::tuple<Buffer, uint64_t /*changedtick*/, int64_t /*firstline*/, int64_t /*lastline*/, ArrayOf(String) /*linedata*/, bool /*more*/>;
using nvim_buf_changedtick_event = std::pair<Buffer, uint64_t /*changedtick*/>;
using nvim_buf_detach_event      = Buffer;

struct nvim_buf_lines_event {
      Buffer              buf;
      Integer             changedtick;
      Integer             firstline;
      Integer             lastline;
      std::vector<String> linedata;
      bool                more;
};

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



/****************************************************************************************/
} // namespace ipc::rpc::neovim
} // namespace MAIN_PACKAGE_NAMESPACE
#endif
