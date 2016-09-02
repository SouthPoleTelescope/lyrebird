import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import curses


class FancyScatterPlot(object):
    def __init__(self, x, y):
        self.labels = len(x) * ['I am a label']
        print self.labels
        self.x = x
        self.y = y
        self.frac_screen = 0.03
        self.figure = plt.figure()
        self.gs = gridspec.GridSpec(3, 3)
        ax1 = plt.subplot(self.gs[0:2,0:2])
        self.ax = ax1
        self.main_plot,  = plt.plot(x, y, '*')
        self.overplot,  = plt.plot([],[], 'ro')
        ax2 = plt.subplot(self.gs[2,0:2], sharex=self.ax)
        self.x_hist = plt.hist(x)
        ax2 = plt.subplot(self.gs[0:2,2], sharey=self.ax)
        self.y_hist = plt.hist(y, orientation = 'horizontal')
        self.figure.canvas.mpl_connect('button_press_event', self.mycall)
    def mycall(self, event):
        self.event = event
        xsize = self.ax.get_xlim()[1]-self.ax.get_xlim()[0]
        ysize = self.ax.get_ylim()[1]-self.ax.get_ylim()[0]
        dist_perc =(((x-event.xdata)/xsize)**2 + ((y-event.ydata)/ysize)**2)**.5
        valid_inds = np.where( dist_perc < self.frac_screen)[0]
        if len(valid_inds) > 0:

            for i in valid_inds:
                print self.labels[i]
                self.overplot.set_xdata( x[valid_inds] )
                self.overplot.set_ydata( y[valid_inds] )
                self.figure.canvas.draw()
        else:
            self.overplot.set_xdata( [] )
            self.overplot.set_ydata( [] )
            self.figure.canvas.draw()

if __name__ == '__main__':
    x = np.array(np.random.rand(100))
    y = np.array(np.random.normal(size = 100))
    print x, y
    plt.ion()

    ugh = FancyScatterPlot(x,y)
    plt.show()
