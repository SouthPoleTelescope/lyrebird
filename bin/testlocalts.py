from spt3g import core, dfmux, calibration

pipe = core.G3Pipeline()
pipe.Add(core.G3Reader, filename = "tcp://localhost:8676")
pipe.Add(core.Dump)
pipe.Run()
