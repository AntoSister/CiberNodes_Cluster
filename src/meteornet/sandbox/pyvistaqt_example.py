import pyvista as pv
from pyvistaqt import BackgroundPlotter

sphere = pv.Sphere()

plotter = BackgroundPlotter()
plotter.add_mesh(sphere)
