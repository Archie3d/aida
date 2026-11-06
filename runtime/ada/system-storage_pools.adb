package body System.Storage_Pools is
    function Reserve (Size : Storage_Elements.Storage_Count) return Address;
    pragma Import (C, Reserve, "__ada_pool_default_allocate");
    procedure Release (Storage_Address : Address);
    pragma Import (C, Release, "__ada_pool_default_deallocate");
    overriding procedure Allocate (Pool : in out Default_Pool;
                                   Storage_Address : out Address;
                                   Size_In_Storage_Elements : Storage_Elements.Storage_Count;
                                   Alignment : Storage_Elements.Storage_Count) is
    begin
        Storage_Address := Reserve (Size_In_Storage_Elements);
    end Allocate;
    overriding procedure Deallocate (Pool : in out Default_Pool;
                                     Storage_Address : Address;
                                     Size_In_Storage_Elements : Storage_Elements.Storage_Count;
                                     Alignment : Storage_Elements.Storage_Count) is
    begin
        Release (Storage_Address);
    end Deallocate;
    overriding function Storage_Size (Pool : Default_Pool) return Storage_Elements.Storage_Count is
    begin
        return Storage_Elements.Storage_Count'Last;
    end Storage_Size;
end System.Storage_Pools;
