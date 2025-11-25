#!/usr/bin/env python3
"""
Convierte el export de Tiled a formato de autos estacionados.
Uso: python3 convert_tiled_parked_cars.py input_tiled.json output.json

Buscamos una capa de objetos llamada "parked_cars" con puntos que tengan:
- Propiedad "horizontal" (bool): true = orientado horizontalmente, false = vertical

No necesitás IDs manuales. Este script NO guarda ids y el juego no los usa.
"""

import json
import sys

def convert_tiled_to_parked_cars(input_file, output_file):
    # Lee el archivo exportado de Tiled
    with open(input_file, 'r') as f:
        tiled_data = json.load(f)
    
    parked_cars = []
    
    # Busca la capa de autos estacionados
    for layer in tiled_data.get('layers', []):
        if layer.get('name') == 'parked_cars' and layer.get('type') == 'objectgroup':
            for obj in layer.get('objects', []):
                # Extrae las propiedades del objeto
                props = {p['name']: p['value'] for p in obj.get('properties', [])}
                
                car = {
                    'x': int(obj['x']),
                    'y': int(obj['y']),
                    'horizontal': props.get('horizontal', True)  # Default: horizontal
                }
                
                parked_cars.append(car)
    
    # No hay IDs; preservar orden de aparición
    
    # Guarda en formato simplificado
    output = {'parked_cars': parked_cars}
    with open(output_file, 'w') as f:
        json.dump(output, f, indent=2)
    
    print(f"✓ Convertidos {len(parked_cars)} autos estacionados")
    print(f"✓ Guardado en: {output_file}")
    
    # Muestra resumen
    horizontal_count = sum(1 for c in parked_cars if c['horizontal'])
    vertical_count = len(parked_cars) - horizontal_count
    print(f"  Horizontales: {horizontal_count}")
    print(f"  Verticales: {vertical_count}")
    
    for idx, car in enumerate(parked_cars):
        orient = "horizontal" if car['horizontal'] else "vertical"
        print(f"  Car #{idx}: ({car['x']}, {car['y']}) → {orient}")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Uso: python3 convert_tiled_parked_cars.py <input_tiled.json> <output.json>")
        print("Ejemplo: python3 convert_tiled_parked_cars.py liberty_city.json data/cities/parked_cars.json")
        sys.exit(1)
    
    convert_tiled_to_parked_cars(sys.argv[1], sys.argv[2])
