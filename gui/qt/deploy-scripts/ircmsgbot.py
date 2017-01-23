#!/usr/bin/env python
# Based on irccat2 example (from Python IRC library)
# by Joel Rosdahl <joel@rosdahl.net>
# Requires Python IRC library

import irc.client
import sys
import time
import threading
import multiprocessing
import logging
import ssl

global msg_ayncs
msg_ayncs = []

class LoggingHandler:
    def __init__(self, *args, **kwargs):
        self.log = logging.getLogger(self.__class__.__name__)

class IRCMsgBot(irc.client.SimpleIRCClient, LoggingHandler):
    def __init__(self, target, message, version_str = None):
        irc.client.SimpleIRCClient.__init__(self)
        self._irctarget = target
        self._message = message
        self._version_str = version_str or "IRCMsgBot v0.1"
        self.disconnected = False
        self.ready_to_send = False
        
        LoggingHandler.__init__(self)
    
    def on_welcome(self, connection, event):
        self.log.debug("Welcome received! Starting send checks...")
        self.ready_to_send = True
        self.reactor.scheduler.execute_after(5, self.check_send)
    
    def on_motd(self, connection, event):
        if self.ready_to_send:
            self.log.debug("MOTD received! Unmarking ready to send...")
        self.ready_to_send = False
    
    def on_motd2(self, connection, event):
        if self.ready_to_send:
            self.log.debug("MOTD received! Unmarking ready to send...")
        self.ready_to_send = False
    
    def on_endofmotd(self, connection, event):
        self.log.debug("End of MOTD received! Marking ready to send...")
        self.ready_to_send = True
    
    def on_disconnect(self, connection, event):
        self.log.debug("Disconnected!")
        self.disconnected = True
        #sys.exit(0)
    
    def on_ctcp(self, connection, event):
        if not self.connection.connected:
            return
        
        nick = event.source.nick
        if event.arguments[0] == "VERSION":
            connection.ctcp_reply(nick, "VERSION " + self._version_str)
        elif event.arguments[0] == "PING":
            if len(event.arguments) > 1:
                connection.ctcp_reply(nick, "PING " + event.arguments[1])
    
    def check_send(self):
        if not self.ready_to_send:
            self.log.debug("Not ready to send yet...")
            self.reactor.scheduler.execute_after(3, self.check_send)
        else:
            self.log.debug("Ready to send, triggering...")
            self.reactor.scheduler.execute_after(3, self.send_it)
    
    def send_it(self):
        if not self.connection.connected:
            return
        self.log.debug("Sending message...")
        
        msg_parts = self._message.split("\n")
        for msg_part in msg_parts:
            self.connection.privmsg(self._irctarget, msg_part)
        
        self.log.debug("Message sent! Disconnecting in 10 seconds...")
        self.reactor.scheduler.execute_after(10, self.disconnect)
    
    def disconnect(self):
        self.log.debug("Sending QUIT...")
        self.connection.quit("See You Space Cowboy... (%s)" % self._version_str)
        
        self.log.debug("QUIT sent! Forcefully disconnecting in 60s...")
        self.reactor.scheduler.execute_after(60, self.hard_disconnect)
    
    def hard_disconnect(self):
        self.log.debug("Forcefully disconnecting...")
        self.connection.disconnect("See You Space Cowboy... (%s)" % self._version_str)

class IRCMsgAsyncBase(LoggingHandler):
    def __init__(self, server, port, nick, target, message,
            use_ssl=False):
        self._server = server
        self._port = port
        self._nick = nick
        self._irctarget = target
        self._message = message
        self._ssl = use_ssl
        
        LoggingHandler.__init__(self)
        
        self.log.debug("Initialized: (%s, %d, %s, %s, %s, %s)" %
            (self._server, self._port, self._nick, self._irctarget,
            self._message,
            "SSL enabled" if self._ssl else "SSL disabled"))
    
    def check_stopnow(self):
        raise NotImplementedError
    
    def stop(self):
        raise NotImplementedError
    
    def run(self):
        self.log.debug("Running with target = %s, message = %s" % (self._irctarget, self._message))
        
        send_irc_message(self._server, self._port, self._nick,
            self._irctarget, self._message, use_ssl = self._ssl,
            periodic_callback = self.check_stopnow)

class IRCMsgThread(IRCMsgAsyncBase, threading.Thread, LoggingHandler):
    def __init__(self, *args, **kwargs):
        self._stopnow = False
        
        IRCMsgAsyncBase.__init__(self, *args, **kwargs)
        threading.Thread.__init__(self)
        LoggingHandler.__init__(self)
        
        self.log.debug("Ready to go! (Thread)")
    
    def check_stopnow(self):
        if self._stopnow:
            self.log.debug("Thread: Detected stop signal, exiting.")
            sys.exit(0)
    
    def stop(self):
        self.log.debug("Thread: Got stop signal...")
        self._stopnow = True

class IRCMsgProcess(IRCMsgAsyncBase, multiprocessing.Process, LoggingHandler):
    def __init__(self, *args, **kwargs):
        self._stopnow = multiprocessing.Event()
        
        IRCMsgAsyncBase.__init__(self, *args, **kwargs)
        multiprocessing.Process.__init__(self)
        LoggingHandler.__init__(self)
        
        self.log.debug("Ready to go! (MP)")
    
    def check_stopnow(self):
        if self._stopnow.is_set():
            self.log.debug("MP: Detected stop signal, exiting.")
            sys.exit(0)
    
    def stop(self):
        self.log.debug("MP: Got stop signal...")
        self._stopnow.set()

# Send an IRC message. Return once complete.
def send_irc_message(server, port, nick, target, message, use_ssl=False,
        periodic_callback=None):
    if periodic_callback: periodic_callback()
    c = IRCMsgBot(target, message)
    if periodic_callback: periodic_callback()
    
    if use_ssl:
        logging.debug("Enabling SSL...")
        conn_factory = irc.connection.Factory(wrapper=ssl.wrap_socket)
    else:
        # Use default
        conn_factory = irc.connection.Factory()
    
    conn_attempts = 0
    
    while conn_attempts < 5:
        try:
            logging.debug("Trying to connect...")
            
            if periodic_callback: periodic_callback()
            c.connect(server, port, nick, connect_factory=conn_factory)
            if periodic_callback: periodic_callback()
            break
        except irc.client.ServerConnectionError as x:
            logging.critical("Failed to connect to IRC server, retrying in 30s.")
            logging.critical(x)
            time.sleep(30)
            conn_attempts += 1
    
    if conn_attempts == 5:
        logging.critical("Failed to connect to IRC server, exiting.")
        sys.exit(1)
    
    if periodic_callback: periodic_callback()
    logging.debug("Starting bot...")
    
    #c.start()
    while not c.disconnected:
        if periodic_callback: periodic_callback()
        
        if not c.disconnected:
            try:
                c.reactor.process_once()
            except irc.client.ServerNotConnectedError:
                logging.debug("Disconnection detected (exception)!")
                pass
        
        if periodic_callback: periodic_callback()
        time.sleep(0.2)
    
    logging.debug("Exited.")

# Asynchronously send IRC message. Note that you must
# call async_wait_all OR async_stop_all in order to properly
# clean up!
def async_send_irc_message(*args, process = False, **kwargs):
    global msg_ayncs
    if process:
        t = IRCMsgProcess(*args, **kwargs)
    else:
        t = IRCMsgThread(*args, **kwargs)
    msg_ayncs.append(t)
    t.start()

def async_wait_all():
    global msg_ayncs
    
    for t in msg_ayncs:
        if t.is_alive():
            t.join()

def async_check_alive():
    for t in msg_ayncs:
        if t.is_alive():
            return True
    return False

def async_stop_all(timeout = 180):
    global msg_ayncs
    
    count = 0
    
    while async_check_alive() and count < timeout:
        logging.debug("Waiting: %d seconds" % (timeout - count))
        time.sleep(1)
        count += 1
    
    if async_check_alive():
        for t in msg_ayncs:
            if t.is_alive():
                t.stop()
    
    async_wait_all()

def main():
    if len(sys.argv) != 5:
        print("Usage: ircmsgbot <server[:port]> <nickname> <target> <msg>")
        print("\ntarget is a nickname or a channel.")
        print("\nTo specify a SSL port, add a + to the front of it.")
        print("\n(Example: +6697)")
        sys.exit(1)
    
    logging.basicConfig(level=logging.DEBUG,
        format='[%(asctime)19s] [%(module)s] [%(name)s.%(funcName)s] %(message)s')
    
    s = sys.argv[1].split(":", 1)
    server = s[0]
    if len(s) == 2:
        if s[1].startswith("+"):
            use_ssl = True
            s[1] = s[1][1:]
        else:
            use_ssl = False
        try:
            port = int(s[1])
        except ValueError:
            print("Error: Erroneous port.")
            sys.exit(1)
    else:
        port = 6667
    nickname = sys.argv[2]
    target = sys.argv[3]
    message = " ".join(sys.argv[4:])
    
    send_irc_message(server, port, nickname, target, message,
        use_ssl = ssl)

if __name__ == "__main__":
    main()
