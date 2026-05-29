from pyvista import examples
import pyvista as pv
print(pv.Report())

mesh = examples.download_dragon()

mesh['scalars'] = mesh.points[:, 1]

mesh.plot(cpos='xy', cmap='plasma')
