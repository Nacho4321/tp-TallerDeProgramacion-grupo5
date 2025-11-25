#!/usr/bin/env python3
"""
Convierte el export de Tiled a formato de waypoints simplificado.
Uso: python3 convert_tiled_waypoints.py input_tiled.json output.json
"""

import json
import sys

def convert_tiled_to_waypoints(input_file, output_file):
    # Lee el archivo exportado de Tiled
    with open(input_file, 'r') as f:
        tiled_data = json.load(f)
    
    waypoints = []
    
    # Busca la capa de waypoints
    for layer in tiled_data.get('layers', []):
        if layer.get('name') == 'npc_waypoints' and layer.get('type') == 'objectgroup':
            for obj in layer.get('objects', []):
                # Extrae las propiedades del objeto
                props = {p['name']: p['value'] for p in obj.get('properties', [])}
                
                wp = {
                    'id': props.get('id', obj.get('id')),
                    'x': int(obj['x']),
                    'y': int(obj['y']),
                    'connections': []
                }
                
                # Parsea las conexiones (formato "1,4,5")
                if 'connections' in props:
                    conn_str = str(props['connections']).strip()
                    if conn_str:
                        wp['connections'] = [int(c.strip()) for c in conn_str.split(',')]
                
                waypoints.append(wp)
    
    # Ordena por ID
    waypoints.sort(key=lambda w: w['id'])
    
    # Guarda en formato simplificado
    output = {'waypoints': waypoints}
    with open(output_file, 'w') as f:
        json.dump(output, f, indent=2)
    
    print(f"✓ Convertidos {len(waypoints)} waypoints")
    print(f"✓ Guardado en: {output_file}")
    
    # Muestra resumen
    for wp in waypoints:
        conns = ','.join(map(str, wp['connections'])) if wp['connections'] else 'none'
        print(f"  Waypoint {wp['id']}: ({wp['x']}, {wp['y']}) → [{conns}]")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Uso: python3 convert_tiled_waypoints.py <input_tiled.json> <output.json>")
        print("Ejemplo: python3 convert_tiled_waypoints.py liberty_city.json data/cities/npc_waypoints.json")
        sys.exit(1)
    
    convert_tiled_to_waypoints(sys.argv[1], sys.argv[2])
