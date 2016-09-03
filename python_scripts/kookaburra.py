import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import socket
import curses

class FancyScatterPlot(object):
    def __init__(self, x, y, labels, 
                 x_label = 'Test X Label',
                 y_label = 'Test Y Label',
                 send_port = 5555,
                 recv_port = 5556,
                 frac_screen = 0.03):
        self.set_labels(labels)
        self.x = x
        self.y = y
        self.frac_screen = frac_screen
        self.figure = plt.figure()
        self.gs = gridspec.GridSpec(3, 3)
        ax1 = plt.subplot(self.gs[0:2,0:2])
        self.ax = ax1
        self.main_plot, = plt.plot(x, y, '*')
        self.overplot, = plt.plot([],[], 'ro')
        self.x_label = x_label
        self.y_label = y_label
        plt.ylabel(self.y_label)

        self.update_hists()

        self.figure.canvas.mpl_connect('button_press_event', self.mycall)
        self.highlighted = []

        UDP_IP = "127.0.0.1"
        UDP_PORT = 5556
        self.sock_listen = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        self.sock_listen.setblocking(False)
        self.sock_listen.bind((UDP_IP, UDP_PORT))
        self.sock_send = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        self.send_port = send_port

    def check_socket(self):
        labels = []
        while 1:
            try:
                data = self.sock_listen.recv(1024)
                labels.append(data[:-1])
            except:
                break
        if len(labels) > 0:
            self.highlight_strs(labels)
            
    def set_labels(self, labels):
        self.labels = labels
        self.label_map = {}
        for i in range(len(labels)):
            self.label_map[labels[i]] = i

    def update_hists(self):
        ax2 = plt.subplot(self.gs[2,0:2], sharex=self.ax)
        self.x_hist = plt.hist(x)
        plt.xlabel(self.x_label)
        ax2 = plt.subplot(self.gs[0:2,2], sharey=self.ax)
        self.y_hist = plt.hist(y, orientation = 'horizontal')

    def update_data(self, x,y):
        self.x = x
        self.y = y
        self.main_plot.set_xdata(self.x)
        self.main_plot.set_ydata(self.y)
        self.update_hists()
        self.highlight_inds(self.highlighted, no_send = True)

    def highlight_inds(self, valid_inds, no_send = True):
        self.highlighted = valid_inds

        if len(valid_inds) > 0:
            for i in valid_inds:
                self.overplot.set_xdata( x[valid_inds] )
                self.overplot.set_ydata( y[valid_inds] )
        else:
            self.overplot.set_xdata( [] )
            self.overplot.set_ydata( [] )
        self.figure.canvas.draw()
        if not no_send:
            for i in valid_inds:
                self.sock_send.sendto(labels[i], ('localhost', self.send_port))

    def highlight_strs(self, strs, no_send = True):
        inds = []
        for s in strs:
            if s in self.label_map:
                inds.append(self.label_map[s])
        self.highlight_inds(inds, no_send = True)
            
    def mycall(self, event):
        self.event = event
        xsize = self.ax.get_xlim()[1]-self.ax.get_xlim()[0]
        ysize = self.ax.get_ylim()[1]-self.ax.get_ylim()[0]
        dist_perc =(((self.x-event.xdata)/xsize)**2 + ((self.y-event.ydata)/ysize)**2)**.5
        valid_inds = np.where( dist_perc < self.frac_screen)[0]
        self.highlight_inds(valid_inds, no_send = False)


if __name__ == '__main__':
    #hashtag awesoem
    ndets = 14705
    x = np.array(np.random.rand(ndets))
    y = np.array(np.random.normal(size = ndets))
    labels =  ['test_label_%s'%i for i in range( len(x) )]
    ugh = FancyScatterPlot(x,y, labels)
    plt.show(block=False)
    while 1:
        plt.pause(0.002)
        x = np.array(np.random.rand(ndets))
        y = np.array(np.random.normal(size = ndets))
        ugh.check_socket()
        ugh.update_data(x,y)

'''
import curses, traceback

#need screen geometry and squid list and squid mapping

def add_squid_info(screen, y, x, 
                   sq_label, sq_label_size,
                   carrier_good, nuller_good, demod_good,
                   temperature_good,
                   bolometer_good, bolo_label = '',
                   neutral_c = 3,
                   good_c = 2,
                   bad_c = 1):
    col_map = {True: curses.color_pair(good_c), 
               False: curses.color_pair(bad_c) }

    current_index = x
    screen.addstr(y, current_index, sq_label, curses.color_pair(neutral_c))
    current_index += sq_label_size

    screen.addstr(y, current_index, 'C', col_map[carrier_good])
    current_index += 1

    screen.addstr(y, current_index, 'N', col_map[nuller_good])
    current_index += 1

    screen.addstr(y, current_index, 'D', col_map[demod_good])
    current_index += 1

    screen.addstr(y, current_index, 'T', col_map[temperature_good])
    current_index += 1

    screen.addstr(y, current_index, 'B', col_map[bolometer_good])
    current_index += 1
    if (not bolometer_good):
        screen.addstr(y, current_index, ' '+bolo_label, col_map[False])

def main(stdscr):
    # Frame the interface area at fixed VT100 size
    screen = stdscr.subwin(0, 160, 0, 0)
    screen.box()
    curses.init_pair(1, curses.COLOR_RED, curses.COLOR_WHITE)
    curses.init_pair(2, curses.COLOR_GREEN, curses.COLOR_BLACK)
    curses.init_pair(3, curses.COLOR_BLUE, curses.COLOR_BLACK)

    squids = ['Sq_%d'%i for i in range(256)]
    squids_per_col = 32
    squid_col_width = 20
    while 1:
        screen.keypad(1)
        for i, s in enumerate(squids):
            add_squid_info(screen, 
                           i % squids_per_col + 1, 
                           1 +squid_col_width * ( i // squids_per_col), 
                           s, 8,
                           True, True, True, 
                           True, False, bolo_label = 'ugh')
        event = screen.getch() 
        if event == ord("q"): break 
        screen.refresh()

if __name__=='__main__':
  try:
      # Initialize curses
      stdscr=curses.initscr()
      #curses.mousemask(curses.ALL_MOUSE_EVENTS)
      curses.start_color()

      # Turn off echoing of keys, and enter cbreak mode,
      # where no buffering is performed on keyboard input
      curses.noecho()
      curses.cbreak()

      # In keypad mode, escape sequences for special keys
      # (like the cursor keys) will be interpreted and
      # a special value like curses.KEY_LEFT will be returned
      stdscr.keypad(1)

      main(stdscr)                    # Enter the main loop
      # Set everything back to normal
      stdscr.keypad(0)
      curses.echo()
      curses.nocbreak()
      curses.endwin()                 # Terminate curses
  except:
      # In event of error, restore terminal to sane state.
      stdscr.keypad(0)
      curses.echo()
      curses.nocbreak()
      curses.endwin()
      traceback.print_exc()           # Print the exception
'''
