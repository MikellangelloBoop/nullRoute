import unreal
level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level.load_level('/Game/Maps/NR_Arcology')
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.DirectionalLight):
        c=a.get_component_by_class(unreal.DirectionalLightComponent)
        c.set_intensity(2.4)
        c.set_light_color(unreal.LinearColor(0.86,0.92,1.0))
        c.set_editor_property('light_source_angle',5.0)
    elif isinstance(a,unreal.SkyLight):
        a.get_component_by_class(unreal.SkyLightComponent).set_intensity(1.1)
    if a.get_actor_label().startswith('NR_Polish_'):
        actors.destroy_actor(a)
# Four bounded accent lights; no shadow maps, replication or actor ticks.
for idx,(x,y,z,color) in enumerate([
    (-1950,0,550,(1.0,.54,.19)),(1910,0,550,(.18,.63,1.0)),
    (0,-1250,550,(.27,.55,1.0)),(0,1250,550,(1.0,.61,.3))]):
    a=actors.spawn_actor_from_class(unreal.RectLight,unreal.Vector(x,y,z),unreal.Rotator(-90,0,0))
    a.set_actor_label('NR_Polish_Accent_'+str(idx))
    c=a.get_component_by_class(unreal.RectLightComponent)
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_intensity(320)
    c.set_light_color(unreal.LinearColor(*color))
    c.set_attenuation_radius(1150)
    c.set_source_width(300)
    c.set_source_height(150)
    c.set_cast_shadows(True)
fog=actors.spawn_actor_from_class(unreal.ExponentialHeightFog,unreal.Vector(0,0,-400))
fog.set_actor_label('NR_Polish_Depth')
c=fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
c.set_editor_property('fog_density',0.009)
c.set_editor_property('fog_height_falloff',0.12)
c.set_editor_property('fog_max_opacity',0.4)
c.set_editor_property('start_distance',1100.0)
c.set_editor_property('fog_inscattering_luminance',unreal.LinearColor(.07,.11,.16))
level.save_current_level()
unreal.log('NR_LIGHTING_POLISHED')
