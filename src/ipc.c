#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include "luaT.h"
#include "workqueue.h"
#include "cliser.h"
#include "map.h"
#include "error.h"
#include "spawn.h"

int ipc_getpid(lua_State *L) {
   pid_t pid = getpid();
   lua_pushinteger(L, pid);
   return 1;
}

int ipc_getppid(lua_State *L) {
   pid_t pid = getppid();
   lua_pushinteger(L, pid);
   return 1;
}

int ipc_gettid(lua_State *L) {
   pthread_t tid = pthread_self();
   lua_pushinteger(L, (intptr_t)tid);
   return 1;
}

int ipc_fork(lua_State *L) {
   pid_t pid = fork();
   lua_pushinteger(L, pid);
   return 1;
}

int ipc_waitpid(lua_State *L) {
   int status;
   pid_t pid = lua_tointeger(L, 1);
   do {
      int ret = waitpid(pid, &status, WUNTRACED | WCONTINUED);
      if (ret < 0) {
         return LUA_HANDLE_ERROR(L, errno);
      }
      if (WIFEXITED(status)) {
         lua_pushinteger(L, WEXITSTATUS(status));
         return 1;
      }
   } while (!WIFEXITED(status) && !WIFSIGNALED(status));
   return 0;
}

static const struct luaL_Reg ipc_routines[] = {
   {"workqueue", workqueue_open},
   {"server", cliser_server},
   {"client", cliser_client},
   {"getpid", ipc_getpid},
   {"getppid", ipc_getppid},
   {"gettid", ipc_gettid},
   {"fork", ipc_fork},
   {"waitpid", ipc_waitpid},
   {"map", map_open},
   {"spawn", spawn_open},
   {NULL, NULL}
};

static const struct luaL_Reg workqueue_routines[] = {
   {"close", workqueue_close},
   {"read", workqueue_read},
   {"write", workqueue_write},
   {"drain", workqueue_drain},
   {NULL, NULL}
};

static const struct luaL_Reg server_routines[] = {
   {"close", cliser_server_close},
   {"clients", cliser_server_clients},
   {"broadcast", cliser_server_broadcast},
   {"recvAny", cliser_server_recv_any},
   {"netStats", cliser_server_net_stats},
   {NULL, NULL}
};

static const struct luaL_Reg server_client_routines[] = {
   {"send", cliser_server_send},
   {"recv", cliser_server_recv},
   {"tag", cliser_server_tag},
   {"id", cliser_server_id},
   {"close", cliser_server_client_close},
   {NULL, NULL}
};

static const struct luaL_Reg client_routines[] = {
   {"close", cliser_client_close},
   {"__gc", cliser_client_close},
   {"send", cliser_client_send},
   {"recv", cliser_client_recv},
   {"recvAsync", cliser_client_recv_async},
   {"retain", cliser_client_retain},
   {"metatablename", cliser_client_metatablename},
   {"netStats", cliser_client_net_stats},
   {NULL, NULL}
};

static const struct luaL_Reg map_routines[] = {
   {"join", map_join},
   {"checkErrors", map_check_errors},
   {NULL, NULL}
};

static const struct luaL_Reg spawn_routines[] = {
   {"stdin", spawn_stdin},
   {"stdout", spawn_stdout},
   {"wait", spawn_wait},
   {"pid", spawn_pid},
   {"running", spawn_running},
   {"__gc", spawn_gc},
   {NULL, NULL}
};

DLL_EXPORT int luaopen_libipc(lua_State *L) {
   signal(SIGPIPE, SIG_IGN);  // don't die for SIGPIPE
   luaL_newmetatable(L, "ipc.workqueue");
   lua_pushstring(L, "__index");
   lua_pushvalue(L, -2);
   lua_settable(L, -3);
   luaT_setfuncs(L, workqueue_routines, 0);
   luaL_newmetatable(L, "ipc.server");
   lua_pushstring(L, "__index");
   lua_pushvalue(L, -2);
   lua_settable(L, -3);
   luaT_setfuncs(L, server_routines, 0);
   luaL_newmetatable(L, "ipc.server.client");
   lua_pushstring(L, "__index");
   lua_pushvalue(L, -2);
   lua_settable(L, -3);
   luaT_setfuncs(L, server_client_routines, 0);
   luaL_newmetatable(L, "ipc.client");
   lua_pushstring(L, "__index");
   lua_pushvalue(L, -2);
   lua_settable(L, -3);
   luaT_setfuncs(L, client_routines, 0);
   luaL_newmetatable(L, "ipc.map");
   lua_pushstring(L, "__index");
   lua_pushvalue(L, -2);
   lua_settable(L, -3);
   luaT_setfuncs(L, map_routines, 0);
   luaL_newmetatable(L, "ipc.spawn");
   lua_pushstring(L, "__index");
   lua_pushvalue(L, -2);
   lua_settable(L, -3);
   luaT_setfuncs(L, spawn_routines, 0);
   Lcliser_CharInit(L);
   Lcliser_ByteInit(L);
   Lcliser_ShortInit(L);
   Lcliser_IntInit(L);
   Lcliser_LongInit(L);
   Lcliser_FloatInit(L);
   Lcliser_DoubleInit(L);
#ifdef USE_CUDA
   Lcliser_CudaInit(L);
#endif
   lua_newtable(L);
   luaT_setfuncs(L, ipc_routines, 0);
   return 1;
}
