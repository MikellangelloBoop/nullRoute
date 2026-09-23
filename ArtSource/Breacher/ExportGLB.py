"""Portable rigid armor rigs, built from the same bone-local geometry used by UE.

The licensed Epic undersuit and animation clips stay in the Unreal project.
No raster processing: the supplied JPG patch reference is embedded unchanged.
"""
import csv
import json
import math
import struct
from pathlib import Path

root = Path(__file__).resolve().parent
bones = list(csv.DictReader((root/'Models/BindPose.csv').open(encoding='utf-8-sig')))

for faction in ['Chronos', 'Police', 'Rebel']:
    binary = bytearray()
    doc = {'asset': {'version': '2.0', 'generator': 'Null Route Breacher authoring pipeline'},
           'scene': 0, 'scenes': [{'nodes': [0]}], 'nodes': [], 'meshes': [],
           'materials': [], 'buffers': [], 'bufferViews': [], 'accessors': []}
    def buffer(data, target=None):
        while len(binary) % 4:
            binary.append(0)
        view = {'buffer': 0, 'byteOffset': len(binary), 'byteLength': len(data)}
        if target:
            view['target'] = target
        doc['bufferViews'].append(view)
        binary.extend(data)
        return len(doc['bufferViews'])-1
    def accessor(rows, kind):
        flat = [v for row in rows for v in row]
        view = buffer(struct.pack('<'+'f'*len(flat), *flat), 34962)
        item = {'bufferView': view, 'componentType': 5126, 'count': len(rows), 'type': kind}
        if kind == 'VEC3':
            item['min'] = [min(v[i] for v in rows) for i in range(3)]
            item['max'] = [max(v[i] for v in rows) for i in range(3)]
        doc['accessors'].append(item)
        return len(doc['accessors'])-1
    colors = {'Chronos': [[.032,.041,.048,1],[.63,.40,.115,1]],
              'Police': [[.035,.070,.105,1],[.46,.57,.62,1]],
              'Rebel': [[.085,.082,.076,1],[.40,.08,.31,1]]}[faction]
    colors += [[.018,.022,.027,1], [.22,.8,.07,1] if faction=='Rebel' else [.015,.65,.9,1], [.17,.20,.22,1], [1,1,1,1]]
    for i, (name, color) in enumerate(zip(['Armor','Trim','Cloth','Glow','Steel','Patch'], colors)):
        doc['materials'].append({'name': name, 'pbrMetallicRoughness': {'baseColorFactor': color, 'metallicFactor': [.65,.78,.03,.25,.85,.12][i], 'roughnessFactor': [.42,.34,.88,.24,.35,.68][i]}})
    doc['materials'][3]['emissiveFactor'] = colors[3][:3]
    jpg_view = buffer((root/f'References/T_Patch{faction}.jpg').read_bytes())
    doc['images'] = [{'bufferView': jpg_view, 'mimeType': 'image/jpeg'}]
    doc['textures'] = [{'source': 0}]
    doc['materials'][5]['pbrMetallicRoughness']['baseColorTexture'] = {'index': 0}
    doc['nodes'].append({'name': faction+' Breacher armor rig', 'rotation': [-math.sqrt(.5),0,0,math.sqrt(.5)], 'scale': [.01,.01,.01], 'children': []})
    for i, bone in enumerate(bones):
        # Reflect Unreal's Y axis into a right-handed basis, then rotate Z-up to Y-up.
        p = [float(bone[k]) for k in ('x','y','z')]
        q = [float(bone[k]) for k in ('qx','qy','qz','qw')]
        doc['nodes'].append({'name': bone['name'], 'translation': [p[0],-p[1],p[2]], 'rotation': [-q[0],q[1],-q[2],q[3]], 'children': []})
    for i, bone in enumerate(bones):
        parent = int(bone['parent'])+1
        doc['nodes'][parent]['children'].append(i+1)
    for path in sorted((root/'Models').glob(f'SM_Breacher_{faction}_*.obj')):
        part = path.stem.removeprefix(f'SM_Breacher_{faction}_')
        if part in ['Badge', 'Sleeve', 'Weapon']:
            continue
        vertices, uvs, groups = [], [], {}
        material = 0
        for line in path.read_text(encoding='utf-8-sig').splitlines():
            bits = line.split()
            if not bits:
                continue
            if bits[0] == 'v':
                x,y,z = map(float,bits[1:]); vertices.append((x,-y,z))
            elif bits[0] == 'vt':
                u,v = map(float,bits[1:]); uvs.append((u,1-v))
            elif bits[0] == 'usemtl':
                material = int(bits[1].split('_')[-1])
            elif bits[0] == 'f':
                face = [tuple(int(v)-1 for v in b.split('/')) for b in bits[1:]]
                groups.setdefault(material, []).append([face[0],face[2],face[1]])
        primitives = []
        for material, faces in groups.items():
            positions, texcoords, normals = [], [], []
            for face in faces:
                a,b,c = [vertices[v[0]] for v in face]
                ab = [b[i]-a[i] for i in range(3)]; ac = [c[i]-a[i] for i in range(3)]
                n = [ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0]]
                length = math.sqrt(sum(v*v for v in n)) or 1
                for v,t in face:
                    positions.append(vertices[v]); texcoords.append(uvs[t]); normals.append([v/length for v in n])
            primitives.append({'attributes': {'POSITION': accessor(positions,'VEC3'), 'NORMAL': accessor(normals,'VEC3'), 'TEXCOORD_0': accessor(texcoords,'VEC2')}, 'material': material})
        mesh_index = len(doc['meshes']); doc['meshes'].append({'name': part, 'primitives': primitives})
        node_index = len(doc['nodes']); doc['nodes'].append({'name': part+' armor', 'mesh': mesh_index})
        joint_index = next(i+1 for i,b in enumerate(bones) if b['name']==part)
        doc['nodes'][joint_index]['children'].append(node_index)
    for node in doc['nodes']:
        if node.get('children') == []:
            del node['children']
    while len(binary) % 4:
        binary.append(0)
    doc['buffers'] = [{'byteLength': len(binary)}]
    encoded = json.dumps(doc, separators=(',',':')).encode()
    encoded += b' '*((-len(encoded))%4)
    glb = struct.pack('<III',0x46546c67,2,12+8+len(encoded)+8+len(binary))
    glb += struct.pack('<II',len(encoded),0x4e4f534a)+encoded
    glb += struct.pack('<II',len(binary),0x004e4942)+binary
    target = root/'Models'/f'Breacher_{faction}.glb'
    target.write_bytes(glb)
    assert len(doc['meshes'])==15 and len(glb)==struct.unpack_from('<I',glb,8)[0]
    print(f'{target.name}: {len(glb):,} bytes, 15 articulated armor parts')
