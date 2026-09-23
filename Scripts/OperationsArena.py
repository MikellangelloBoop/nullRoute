import unreal
level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level.load_level('/Game/Maps/NR_Arcology')
for a in actors.get_all_level_actors():
 if a.get_actor_label().startswith('NR_Operations_'):actors.destroy_actor(a)
count=0
def mesh(name,pos,yaw=0,collision=True):
 global count
 a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*pos),unreal.Rotator(pitch=0,yaw=yaw,roll=0));a.set_actor_label('NR_Operations_'+name+'_'+str(count));count+=1
 c=a.static_mesh_component;c.set_static_mesh(unreal.load_asset('/Game/Art/Meshes/'+name));c.set_collision_profile_name('BlockAll' if collision else 'NoCollision');c.set_mobility(unreal.ComponentMobility.STATIC);a.tags=['SurfaceMetal'];return a
def label(text,pos,yaw=0,size=24,color=(120,218,229)):
 global count
 a=actors.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(*pos),unreal.Rotator(pitch=0,yaw=yaw,roll=0));a.set_actor_label('NR_Operations_Label_'+str(count));count+=1
 c=a.get_component_by_class(unreal.TextRenderComponent);c.set_text(text);c.set_world_size(size);c.set_text_render_color(unreal.Color(*color,255));c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
for x,y,yaw in [(-1300,1020,0),(1100,-1070,180),(-530,1550,0),(865,1400,180)]:mesh('SM_TacticalConsole',(x,y,0),yaw)
for x in [-2190,2090]:
 mesh('SM_SectorGate',(x,0,0),0 if x<0 else 180)
 label('EXTRACTION / 01' if x<0 else 'ELARA / CORE',(x+(-41 if x>0 else 41),0,310),180 if x>0 else 0,18,(231,176,89) if x<0 else (110,210,235))
for x in [-2130,2130]:
 for y in [-800,800]:mesh('SM_RouteBeacon',(x,y,0))
label('LOW PROFILE / KEEP MOVING',(-2370,1100,270),0,22)
label('RELAY OVERRIDE\nSECURITY BLACKOUT: 12s',(0,728,565),-90,19,(230,176,93))
label('01 / INFILTRATE   02 / RECOVER   03 / EXTRACT',(-2420,-300,395),0,22)
# Soften the previous overexposed concrete while retaining clear silhouettes and existing dynamic shadows.
for a in actors.get_all_level_actors():
 if isinstance(a,unreal.DirectionalLight):
  c=a.get_component_by_class(unreal.DirectionalLightComponent);c.set_intensity(1.65);c.set_light_color(unreal.LinearColor(.90,.94,1.0));c.set_editor_property('light_source_angle',3.0)
 elif isinstance(a,unreal.SkyLight):a.get_component_by_class(unreal.SkyLightComponent).set_intensity(.8)
 elif isinstance(a,unreal.PointLight):
  c=a.get_component_by_class(unreal.PointLightComponent);c.set_intensity(min(c.get_editor_property('intensity'),100000))
level.save_current_level()
unreal.log('NR_OPERATIONS_MAP_COMPLETE '+str(count))
