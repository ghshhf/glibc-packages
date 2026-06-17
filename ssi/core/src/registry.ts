/**
 * SSI-CORE: Component Registry
 *
 * Central registry for all system components. Maintains the
 * mapping from component name/UUID to component instance.
 * Used by the SSI Service Bus for message routing.
 */

import {
  ISsiComponent,
  SsiComponentState,
  SsiRegistryEntry,
  UUID,
  timestampUs,
} from './types';

export class ComponentRegistry {
  private entries = new Map<UUID, SsiRegistryEntry>();
  private instances = new Map<UUID, ISsiComponent>();
  private byName = new Map<string, UUID>();
  private byType = new Map<string, Set<UUID>>();

  /** Register a component */
  register(component: ISsiComponent): void {
    const id = component.getId();
    const entry: SsiRegistryEntry = {
      uuid: id.uuid,
      name: id.name,
      type: id.type,
      version: id.version,
      providesInterfaces: [],  // filled by component
      requiresInterfaces: [],  // filled by component
      state: component.getState(),
      registeredAt: timestampUs(),
    };

    this.entries.set(id.uuid, entry);
    this.instances.set(id.uuid, component);
    this.byName.set(id.name, id.uuid);

    if (!this.byType.has(id.type)) {
      this.byType.set(id.type, new Set());
    }
    this.byType.get(id.type)!.add(id.uuid);
  }

  /** Unregister a component */
  unregister(uuid: UUID): void {
    const entry = this.entries.get(uuid);
    if (entry) {
      this.byName.delete(entry.name);
      const typeSet = this.byType.get(entry.type);
      if (typeSet) {
        typeSet.delete(uuid);
        if (typeSet.size === 0) this.byType.delete(entry.type);
      }
      this.entries.delete(uuid);
      this.instances.delete(uuid);
    }
  }

  /** Find component by UUID */
  findByUuid(uuid: UUID): ISsiComponent | undefined {
    return this.instances.get(uuid);
  }

  /** Find component by name */
  findByName(name: string): ISsiComponent | undefined {
    const uuid = this.byName.get(name);
    return uuid ? this.instances.get(uuid) : undefined;
  }

  /** Find all components of a given type */
  findByType(type: string): ISsiComponent[] {
    const uuids = this.byType.get(type);
    if (!uuids) return [];
    return Array.from(uuids)
      .map((uuid) => this.instances.get(uuid))
      .filter((c): c is ISsiComponent => c !== undefined);
  }

  /** Get all registry entries */
  listEntries(): SsiRegistryEntry[] {
    return Array.from(this.entries.values());
  }

  /** Update state for a component */
  updateState(uuid: UUID, state: SsiComponentState): void {
    const entry = this.entries.get(uuid);
    if (entry) {
      entry.state = state;
    }
  }

  /** Get count of registered components */
  get size(): number {
    return this.entries.size;
  }

  /** Check if a component is registered */
  has(uuid: UUID): boolean {
    return this.entries.has(uuid);
  }

  /** Clear all entries */
  clear(): void {
    this.entries.clear();
    this.instances.clear();
    this.byName.clear();
    this.byType.clear();
  }
}
