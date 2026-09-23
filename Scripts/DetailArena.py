import unreal
level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level.load_level('/Game/Maps/NR_Arcology')
for a in actors.get_all_level_actors():
    if a.get_actor_label().startswith('NR_Detail_'):actors.destroy_actor(a)
cube=unreal.load_asset('/Engine/BasicShapes/Cube')
counter=0
def prop(mesh,pos,scale=(1,1,1),rot=(0,0,0),mat=None,shadow=True):
    global counter
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*pos),unreal.Rotator(*rot))
    a.set_actor_label('NR_Detail_'+str(counter));counter+=1
    c=a.static_mesh_component;c.set_static_mesh(mesh);c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    c.set_cast_shadow(shadow);c.set_editor_property('mobility',unreal.ComponentMobility.STATIC)
    a.set_actor_scale3d(unreal.Vector(*scale))
    if mat:c.set_material(0,mat)
    return a
def box(pos,size,mat,shadow=True):return prop(cube,pos,tuple(v/100 for v in size),mat=mat,shadow=shadow)
steel=unreal.load_asset('/Game/Art/Materials/M_WeaponSteel')
white=unreal.load_asset('/Game/Art/Materials/M_Ceramic')
amber=unreal.load_asset('/Game/Art/Materials/M_WeaponAccent')
cyan=unreal.load_asset('/Game/Art/Materials/M_CyanLight')
vent=unreal.load_asset('/Game/Art/Meshes/SM_WallVent')
pipe=unreal.load_asset('/Game/Art/Meshes/SM_ServicePipe')
tray=unreal.load_asset('/Game/Art/Meshes/SM_CableTray')
# Overhead services and wall ventilation add scale; all are cosmetic and leave routes clear.
for side in [-1,1]:
    for x in range(-2000,2001,400):
        prop(tray,(x,side*1500,765),(2,1,1))
        if x%800==0:
            box((x,side*1500,824),(5,5,112),steel)
            prop(vent,(x,side*1910,420),rot=(0,90 if side<0 else -90,0))
    for y in [-550,550]:
        prop(pipe,(side*2300,y,470),(3,1,1),rot=(0,90,0))
        box((side*2300,y,475),(12,28,12),amber)
# Walkway borders, gate warning stripes and floor service panels use low-profile meshes.
for side in [-1,1]:
    for i in range(16):
        x=-1800+i*235
        box((x,side*1730,.8),(155,4,.8),white,False)
    for y in range(-300,301,80):
        prop(cube,(side*1950,y,1.0),(.28,.45,.01),(0,35,0),amber,False)
    for y in [-1740,1740]:
        box((side*900,y,1),(170,125,1),steel,False)
        for j in range(9):box((side*900-68+j*17,y,1.6),(5,113,.5),white,False)
for a in actors.get_all_level_actors():
    if not isinstance(a,unreal.StaticMeshActor):continue
    c=a.static_mesh_component
    if c.static_mesh and c.get_collision_enabled()!=unreal.CollisionEnabled.NO_COLLISION:
        materials=[c.get_material(i).get_name() if c.get_material(i) else '' for i in range(c.get_num_materials())]
        if any(n in ['M_FloorPanel','M_Graphite','M_WeaponSteel'] for n in materials):
            tags=list(a.tags)
            if 'SurfaceMetal' not in [str(t) for t in tags]:tags.append('SurfaceMetal');a.tags=tags
level.save_current_level()
unreal.log('NR_DETAIL_ARENA_COMPLETE '+str(counter))
