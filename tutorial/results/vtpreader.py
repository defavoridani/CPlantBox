import vtk

reader = vtk.vtkXMLPolyDataReader()
path = "example_5a.vtp" #path or name of the vtp output
reader.SetFileName(path)
reader.Update()
 
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputData(reader.GetOutput())
 
plantActor = vtk.vtkActor()
plantActor.SetMapper( mapper )

ren1= vtk.vtkRenderer()
ren1.AddActor( plantActor )
ren1.SetBackground( 0.1, 0.2, 0.4 )

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren1 )
renWin.SetSize( 600, 600 )


iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
    
iren.Initialize()
renWin.Render()
iren.Start()