import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import curses


class FancyScatterPlot(object):
    def __init__(self, x, y, labels, frac_screen = 0.03):
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

        ax2 = plt.subplot(self.gs[2,0:2], sharex=self.ax)
        self.x_hist = plt.hist(x)
        ax2 = plt.subplot(self.gs[0:2,2], sharey=self.ax)
        self.y_hist = plt.hist(y, orientation = 'horizontal')

        self.figure.canvas.mpl_connect('button_press_event', self.mycall)
        self.highlighted = []

    def set_labels(self, labels):
        self.labels = labels
        self.label_map = {}
        for i in range(len(labels)):
            self.label_map[labels[i]] = i

    def update_data(self, x,y):
        self.x = x
        self.y = y
        self.main_plot.set_xdata(self.x)
        self.main_plot.set_ydata(self.y)
        ax2 = plt.subplot(self.gs[2,0:2], sharex=self.ax)
        self.x_hist = plt.hist(x)
        ax2 = plt.subplot(self.gs[0:2,2], sharey=self.ax)
        self.y_hist = plt.hist(y, orientation = 'horizontal')
        self.highlight_inds(self.highlighted)

    def highlight_inds(self, valid_inds):
        self.highlighted = valid_inds

        if len(valid_inds) > 0:
            for i in valid_inds:
                self.overplot.set_xdata( x[valid_inds] )
                self.overplot.set_ydata( y[valid_inds] )
        else:
            self.overplot.set_xdata( [] )
            self.overplot.set_ydata( [] )
        self.figure.canvas.draw()

    def highlight_strs(self, strs):
        inds = []
        for s in strs:
            if s in self.label_map:
                inds.append(self.label_map[s])
        self.highlight_inds(inds)
            
    def mycall(self, event):
        print 'in mycall'
        self.event = event
        xsize = self.ax.get_xlim()[1]-self.ax.get_xlim()[0]
        ysize = self.ax.get_ylim()[1]-self.ax.get_ylim()[0]
        dist_perc =(((self.x-event.xdata)/xsize)**2 + ((self.y-event.ydata)/ysize)**2)**.5
        valid_inds = np.where( dist_perc < self.frac_screen)[0]
        self.highlight_inds(valid_inds)

if __name__ == '__main__':
    #hashtag awesoem
    x = np.array(np.random.rand(100))
    y = np.array(np.random.normal(size = 100))
    labels = len(x) * ['I am a label']
    ugh = FancyScatterPlot(x,y, labels)
    plt.show(block=False)
    while 1:
        plt.pause(0.002)
        x = np.array(np.random.rand(100))
        y = np.array(np.random.normal(size = 100))
        ugh.update_data(x,y)
