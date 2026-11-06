with System;
with System.Storage_Pools;
with System.Storage_Elements;
package Storage_Pool_Globals is
    subtype Count is System.Storage_Elements.Storage_Count;
    type Heap_Pool is new System.Storage_Pools.Root_Storage_Pool with record
        Outstanding : Natural := 0;
    end record;
    overriding procedure Allocate (Pool : in out Heap_Pool; Storage_Address : out System.Address;
        Size_In_Storage_Elements, Alignment : Count);
    overriding procedure Deallocate (Pool : in out Heap_Pool; Storage_Address : System.Address;
        Size_In_Storage_Elements, Alignment : Count);
    overriding function Storage_Size (Pool : Heap_Pool) return Count;
    Pool : Heap_Pool;
    type Ref is access Integer;
    for Ref'Storage_Pool use Pool;
    procedure Drop (Item : in out Ref);
end Storage_Pool_Globals;
