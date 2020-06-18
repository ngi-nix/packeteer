/**
 * Unregister connector for READ events, based on the connector type.
 **/
inline void
unregister_socket_from_read_events(iocp_socket_select & sock_select, connector & conn)
{
  DLOG("No longer interested when socket-like handle is readable.");
  // FIXME sock_select.configure_socket(conn.get_read_handle().sys_handle(), 0);
}



/**
 * Register connector for READ events, based on the connector type.
 **/
inline void
register_socket_for_read_events(iocp_socket_select & sock_select, connector & conn)
{
  DLOG("Request notification when socket-like handle becomes readable.");
  // FIXME sock_select.configure_socket(conn.get_read_handle().sys_handle(),
      // FIXME all wrong. Use packeteer events here
      //FD_ACCEPT | FD_CONNECT | FD_READ | FD_CLOSE);
}




