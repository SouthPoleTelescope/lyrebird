import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import socket
import curses

class FancyScatterPlot(object):
    def __init__(self, x, y, labels, 
                 x_label = 'Test X Label',
                 y_label = 'TEst Y Label',
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
