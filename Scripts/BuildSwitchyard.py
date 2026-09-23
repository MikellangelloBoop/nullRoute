"""Author a separate, reproducible combat map; keep NR_Arcology available for regression tests."""
import json,math
from pathlib import Path
import unreal

level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level.load_level('/Game/Maps/NR_Arcology')
if not unreal.EditorLoadingAndSavingUtils.save_map(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'/Game/Maps/NR_Switchyard'):
    raise RuntimeError('Cannot create Switchyard map')
if not level.load_level('/Game/Maps/NR_Switchyard'):
    raise RuntimeError('Cannot load the new map copy')
# Only this newly copied level is edited. Gameplay fixtures, destruction, navigation and starts survive.
for a in actors.get_all_level_actors():
    if isinstance(a,(unreal.StaticMeshActor,unreal.TextRenderActor)) or a.get_class().get_name() in ['NRActivity','NRTacticalDoor']:
        actors.destroy_actor(a)

cube=unreal.load_asset('/Engine/BasicShapes/Cube')
mats={n:unreal.load_asset('/Game/Art/Materials/M_'+n) for n in ['Concrete','FloorPanel','Graphite','Ceramic','WeaponSteel','AmberLight','CyanLight','WeaponAccent']}
for name,m in mats.items():
    if not m:raise RuntimeError('Missing material '+name)
count=0
def box(name,p,size,mat='Concrete',yaw=0,pitch=0,collision=True):
    global count
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*p),unreal.Rotator(pitch=pitch,yaw=yaw,roll=0))
    a.set_actor_label('SY_'+name+'_'+str(count));count+=1
    c=a.static_mesh_component;c.set_static_mesh(cube);c.set_material(0,mats[mat]);c.set_mobility(unreal.ComponentMobility.STATIC)
    c.set_collision_profile_name('BlockAll' if collision else 'NoCollision');c.set_cast_shadow(collision)
    a.set_actor_scale3d(unreal.Vector(*(v/100 for v in size)));a.tags=['SurfaceMetal'] if mat!='Concrete' else []
    return a
def prop(name,p,yaw=0,scale=(1,1,1),collision=True):
    global count
    asset=unreal.load_asset('/Game/Art/Meshes/'+name)
    if not asset:raise RuntimeError('Missing prop '+name)
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*p),unreal.Rotator(pitch=0,yaw=yaw,roll=0))
    a.set_actor_label('SY_'+name+'_'+str(count));count+=1;c=a.static_mesh_component;c.set_static_mesh(asset)
    c.set_collision_profile_name('BlockAll' if collision else 'NoCollision');c.set_mobility(unreal.ComponentMobility.STATIC)
    a.set_actor_scale3d(unreal.Vector(*scale));a.tags=['SurfaceMetal'];return a
def text(label,p,yaw=0,size=24,color=(136,218,226)):
    global count
    a=actors.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(*p),unreal.Rotator(pitch=0,yaw=yaw,roll=0));a.set_actor_label('SY_Sign_'+str(count));count+=1
    c=a.get_component_by_class(unreal.TextRenderComponent);c.set_text(label);c.set_world_size(size);c.set_text_render_color(unreal.Color(*color,255));c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    angle=math.radians(yaw)
    box('SignBack',(p[0]-math.cos(angle)*3,p[1]-math.sin(angle)*3,p[2]+size*.4),(4,max(120,len(label)*size*.57),size*1.5),'Graphite',yaw=yaw,collision=False)
def light(p,color,intensity=500,radius=1050):
    global count
    a=actors.spawn_actor_from_class(unreal.RectLight,unreal.Vector(*p),unreal.Rotator(pitch=-90,yaw=0,roll=0));a.set_actor_label('SY_Light_'+str(count));count+=1
    c=a.get_component_by_class(unreal.RectLightComponent);c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_light_color(unreal.LinearColor(*color));c.set_intensity(intensity);c.set_attenuation_radius(radius);c.set_source_width(240);c.set_source_height(90);c.set_cast_shadows(True)

# Architectural shell and panel courses. The ground top is exactly Z=0.
box('Foundation',(0,0,-40),(5000,4000,80),'FloorPanel')
for y in [-2015,2015]:box('Perimeter',(0,y,380),(5060,80,840))
for x in [-2515,2515]:box('Perimeter',(x,0,380),(80,4000,840))
for side in [-1,1]:
    for x in range(-2250,2300,450):
        box('WallCladding',(x,side*1960,245),(416,20,400),'Ceramic')
        box('WallPlinth',(x,side*1945,58),(416,24,116),'Graphite')
        box('WallSeam',(x,side*1942,455),(416,6,4),'WeaponSteel',collision=False)
        prop('SM_WallVent',(x,side*1918,520),90 if side<0 else -90,collision=False)
    box('Eave',(0,side*1800,780),(4980,430,35),'Graphite')
    for x in [-2180,-1200,0,1200,2180]:
        box('Buttress',(x,side*1890,380),(48,100,760),'Graphite')
        box('VerticalLight',(x+27,side*1839,380),(5,8,370),'AmberLight' if side<0 else 'CyanLight',collision=False)

# Roof panels, skylight slots and supported services give the halls an architectural enclosure.
for side in [-1,1]:
    for x in [-2150,-1300,-450,400,1250,2100]:
        box('RoofPanel',(x,side*1370,780),(610,1050,32),'Graphite')
        box('RoofRib',(x,side*1370,757),(26,1060,32),'WeaponSteel')
    for x in [-1350,-400,550,1420]:box('LampSuspension',(x,side*1300,708),(3,3,135),'WeaponSteel',collision=False)
    for x in [-1130,1130]:
        box('AtriumColumn',(x,side*770,370),(42,42,740),'Graphite')
        box('AtriumAccent',(x-24,side*770,420),(3,18,190),'CyanLight',collision=False)
for x in [-1130,1130]:box('AtriumBeam',(x,0,760),(55,1610,42),'WeaponSteel')
box('SignHanger',(-1070,0,736),(6,6,72),'WeaponSteel',collision=False)

# Three recognizable routes. Two cross-connections break the long lane sightlines.
for y in [-820,820]:
    for x,width in [(-1200,520),(150,1100),(1340,420)]:
        box('RoomWall',(x,y,240),(width,44,480),'Ceramic')
        box('RoomPlinth',(x,y,65),(width+8,52,130),'Graphite')
        box('ServiceStrip',(x,y+(-28 if y<0 else 28),334),(width-30,4,5),'AmberLight' if y<0 else 'CyanLight',collision=False)
    for x in [-760,900]:
        box('CrosslinkLintel',(x,y,470),(320,65,70),'Graphite')

# North: coolant machinery with broad playable pockets, never a line of identical crates.
for x,y,yaw in [(-1110,1570,0),(-70,1640,0),(1170,1510,180)]:
    prop('SM_CoolingCore',(x,y,0),yaw,(1.05,1.05,1.7))
    prop('SM_ServicePipe',(x,y,570),90,(2.4,1.2,1.2),False)
for x,y in [(-850,1130),(460,1380)]:prop('SM_ServiceDivider',(x,y,0),90,(1.1,1,1.05))
box('CoolantEntryBaffle',(-1430,1400,105),(170,220,210),'Graphite')
box('NorthOverlook',(1530,1360,60),(240,450,120),'Graphite')

# South: staggered service alcoves and accessible cover of two heights.
for x,y,yaw in [(-1020,-1580,0),(260,-1430,180),(1170,-1700,0)]:
    prop('SM_ServerRack',(x,y,0),yaw,(.9,.9,.85))
for x,y in [(-630,-1130),(700,-1180)]:
    box('ServiceWorktop',(x,y,80),(250,95,160),'Graphite');box('WorktopEdge',(x,y,164),(260,103,8),'WeaponSteel')
prop('SM_ServiceDivider',(-1420,-1400,0),90,(.9,1,1))
prop('SM_CoverCrate',(950,-1590,0),25,(1,1,.75))

# Central exchange: the existing Chaos bridges remain destructible. Leave their footprints open.
box('ExchangePlatform',(0,0,384),(790,790,32),'FloorPanel')
for side in [-1,1]:
    box('PlatformLanding',(side*975,0,384),(250,790,32),'FloorPanel')
    # Ramps have a 1:2 rise. Floor overlaps at each end remove collision seams.
    box('AccessRamp',(side*1030,side*610,190),(925,235,28),'FloorPanel',pitch=-side*26.2)
    box('RampLanding',(side*630,side*540,384),(280,380,32),'FloorPanel')
    for x in [-950,-150,150,950]:
        box('RailPost',(x,side*470,445),(10,10,120),'WeaponSteel')
    for x,width in [(-950,220),(0,550),(950,220)]:
        box('SafetyRail',(x,side*470,504),(width,10,10),'WeaponSteel')
    # Ground-level server islands leave an unobstructed central path and cross passages.
    for x in [-720,140,780]:prop('SM_ServerRack',(x,side*590,0),90 if side<0 else -90,(.8,.8,1))
    box('BridgeLight',(0,side*384,367),(670,8,8),'CyanLight',collision=False)
prop('SM_TacticalConsole',(-100,170,400),180,collision=False)

# East vault and west staging are kept clear around multiplayer spawn grids and practice targets.
for side in [-1,1]:
    x=side*2350
    box('SpawnHeader',(x,0,450),(65,1070,100),'Graphite')
    text('04 / ELARA VAULT' if side>0 else '00 / INSERTION', (x-side*40,0,447),180 if side>0 else 0,31,(121,214,225) if side>0 else (235,172,83))
    for y in [-880,880]:prop('SM_RouteBeacon',(side*2240,y,0))
for y in [-1120,1120]:box('VaultPartition',(1570,y,245),(55,640,490),'Ceramic')
prop('SM_SectorGate',(1740,0,0),180)
for y in [-410,410]:box('VaultWing',(1780,y,190),(90,210,380),'Graphite')

# Floor wayfinding, restrained fixtures, overhead services and junction numbers.
for side in [-1,1]:
    for x in range(-1350,1500,300):
        box('LaneMark',(x,side*1050,1),(155,6,1),'Ceramic',collision=False)
    for x in [-1350,-400,550,1420]:
        prop('SM_CableTray',(x,side*1300,680),0,(3,1,1),False)
        box('LightHousing',(x,side*1300,644),(250,52,14),'Graphite',collision=False)
        box('Diffuser',(x,side*1300,635),(210,32,3),'Ceramic',collision=False)
        light((x,side*1300,622),(.70,.86,1) if side>0 else (1,.77,.5),380,920)
    text('01 / COOLANT' if side>0 else '03 / SERVICE',(-1480,side*1300,395),180,27)
text('02 / SIGNAL EXCHANGE',(-1070,0,680),180,30)
text('RELAY OVERRIDE',(-100,170,580),180,18,(239,173,80))
text('NORTH  >   COOLANT',(-2340,1210,330),0,23)
text('SOUTH  >   SERVICE',(-2340,-1290,330),0,23,(236,177,91))
light((0,0,810),(.65,.84,1),650,1400)

activity_class=unreal.load_class(None,'/Script/NRGameplay.NRActivity')
for i,(kind,p,yaw) in enumerate([
    (unreal.NRActivityKind.SUPPLY,(-2040,1100,52),0),(unreal.NRActivityKind.SUPPLY,(2030,-1220,52),180),
    (unreal.NRActivityKind.RELAY,(-60,50,452),0),
    (unreal.NRActivityKind.RELAY,(770,-1730,52),90),
    (unreal.NRActivityKind.TARGET,(-1050,-1810,44),-90),(unreal.NRActivityKind.TARGET,(-810,-1810,44),-90),(unreal.NRActivityKind.TARGET,(-570,-1810,44),-90)]):
    a=actors.spawn_actor_from_class(activity_class,unreal.Vector(*p),unreal.Rotator(pitch=0,yaw=yaw,roll=0));a.set_actor_label('SY_Activity_'+str(i));a.set_editor_property('kind',kind)

door_class=unreal.load_class(None,'/Script/NRGameplay.NRTacticalDoor')
for i,(p,yaw) in enumerate([((-760,900,90),90),((900,-900,90),-90)]):
    a=actors.spawn_actor_from_class(door_class,unreal.Vector(*p),unreal.Rotator(pitch=0,yaw=yaw,roll=0));a.set_actor_label('SY_Shutter_'+str(i))
    a.set_editor_property('gate_offset',unreal.Vector(0,0,60));a.set_editor_property('gate_extent',unreal.Vector(18,150,150))
    # The control pedestal is beside the opening, while the panel fills its cross connection.
    a.set_actor_location(unreal.Vector(p[0]-250,p[1],90),False,False)
    a.set_editor_property('gate_offset',unreal.Vector(-80,-250 if yaw==90 else 250,60))

for a in actors.get_all_level_actors():
    if isinstance(a,unreal.DirectionalLight):
        c=a.get_component_by_class(unreal.DirectionalLightComponent);c.set_intensity(1.8);c.set_light_color(unreal.LinearColor(.87,.94,1));c.set_editor_property('light_source_angle',2.0)
        a.set_actor_rotation(unreal.Rotator(pitch=-52,yaw=-35,roll=0),False)
    elif isinstance(a,unreal.PointLight):a.get_component_by_class(unreal.PointLightComponent).set_intensity(9000)
    elif isinstance(a,unreal.SkyLight):a.get_component_by_class(unreal.SkyLightComponent).set_intensity(.65)

if not level.save_current_level():raise RuntimeError("Map save failed")
Path(unreal.Paths.project_saved_dir(),'SwitchyardBuild.json').write_text(json.dumps({'map':'NR_Switchyard','decorated_components':count,'routes':['Coolant','Exchange','Service'],'shutters':2,'activities':7},indent=2),encoding='utf-8')
unreal.log('NR_SWITCHYARD_BUILD_COMPLETE '+str(count))
