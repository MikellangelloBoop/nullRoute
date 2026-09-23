import unreal
level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level.load_level('/Game/Maps/NR_Arcology')
for a in actors.get_all_level_actors():
 if a.get_actor_label().startswith('NR_Expansion_'):actors.destroy_actor(a)
idx=0
def mesh(name,pos,rot=0,scale=(1,1,1),collision=True):
 global idx
 a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*pos),unreal.Rotator(pitch=0,yaw=rot,roll=0));a.set_actor_label('NR_Expansion_Prop_'+str(idx));idx+=1
 c=a.static_mesh_component;c.set_static_mesh(unreal.load_asset('/Game/Art/Meshes/'+name));c.set_collision_profile_name('BlockAll' if collision else 'NoCollision');c.set_mobility(unreal.ComponentMobility.STATIC)
 a.set_actor_scale3d(unreal.Vector(*scale));a.tags=['SurfaceMetal'];return a
def label(text,pos,yaw=0,size=38,color=(100,210,230)):
 global idx
 a=actors.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(*pos),unreal.Rotator(pitch=0,yaw=yaw,roll=0));a.set_actor_label('NR_Expansion_Label_'+str(idx));idx+=1
 c=a.get_component_by_class(unreal.TextRenderComponent);c.set_text(text);c.set_world_size(size);c.set_text_render_color(unreal.Color(*color,255));c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
# Coolant hall in the northern flank: alternating covers create broken sightlines.
for x,y,rot in [(-1050,1450,20),(-200,1510,-20),(560,1400,20),(1230,1530,-15)]:
 mesh('SM_ServiceDivider',(x,y,0),rot)
for x in [-1100,0,1100]:mesh('SM_CoolingCore',(x,1810,0),scale=(.8,.8,1.7))
# A southern maintenance route has smaller machinery, open access to the existing stairs.
for x,y in [(-900,-1610),(50,-1560),(900,-1690)]:mesh('SM_CoolingCore',(x,y,0),scale=(.63,.63,.72))
for x in [-700,600]:mesh('SM_ServiceDivider',(x,-1370,0),-25,scale=(.75,.8,.7))
# Articulated frames distinguish the three zones without adding an expensive dynamic light grid.
for x in [-1700,-650,650,1700]:
 for y in [-1780,1780]:mesh('SM_ServicePipe',(x,y,560),rot=90,scale=(2,1.5,1.5),collision=False)
label('01 / COOLANT HALL',(-2420,1500,300),0)
label('02 / SIGNAL EXCHANGE',(-250,400,660),180,35)
label('03 / LIVE FIRE',(-300,-1910,275),90,40,(230,168,70))
label('SUPPLY',(-2150,1100,195),0,26)
label('RELAY / HOLD POSITION',(0,680,540),180,24)
cls=unreal.load_class(None,'/Script/NRGameplay.NRActivity')
def activity(kind,pos,yaw=0):
 global idx
 a=actors.spawn_actor_from_class(cls,unreal.Vector(*pos),unreal.Rotator(pitch=0,yaw=yaw,roll=0));a.set_actor_label('NR_Expansion_Activity_'+str(idx));idx+=1
 a.set_editor_property('kind',kind)
activity(unreal.NRActivityKind.SUPPLY,(-2120,1100,52),180)
activity(unreal.NRActivityKind.SUPPLY,(2070,-1250,52))
activity(unreal.NRActivityKind.RELAY,(0,680,392))
for x in [-420,-180,60]:activity(unreal.NRActivityKind.TARGET,(x,-1810,44),-90)
# A slightly warmer sunlight balances the colder machine bay.
for a in actors.get_all_level_actors():
 if isinstance(a,unreal.DirectionalLight):
  c=a.get_component_by_class(unreal.DirectionalLightComponent);c.set_light_color(unreal.LinearColor(.97,.91,.79));c.set_intensity(2.1)
level.save_current_level()
unreal.log('NR_EXPANSION_MAP_COMPLETE '+str(idx))
