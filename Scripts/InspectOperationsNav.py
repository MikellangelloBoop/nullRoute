import unreal
l=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);l.load_level('/Game/Maps/NR_Arcology')
a=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for x in a.get_all_level_actors():
 n=x.get_class().get_name()
 if 'Nav' in n:
  unreal.log('NR_NAV_ACTOR '+x.get_name()+' '+n+' '+str(x.get_actor_bounds(False)))
  for prop in ['runtime_generation','can_be_main_nav_data','agent_radius','agent_height','tile_size_uu','supported_agents']:
   try:unreal.log('NR_NAV_PROP '+prop+' '+str(x.get_editor_property(prop)))
   except Exception:pass
unreal.log('NR_NAV_WORLD_CONFIG '+str(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_world_settings().get_editor_property('navigation_system_config')))
