with Ada.Unchecked_Deallocation;
package body Storage_Pool_Globals is
    function Reserve (Size : Count) return System.Address;
    pragma Import (C, Reserve, "__ada_pool_default_allocate");
    procedure Release (Address : System.Address);
    pragma Import (C, Release, "__ada_pool_default_deallocate");
    overriding procedure Allocate (Pool : in out Heap_Pool; Storage_Address : out System.Address;
        Size_In_Storage_Elements, Alignment : Count) is
    begin
        Storage_Address := Reserve (Size_In_Storage_Elements);
        Pool.Outstanding := Pool.Outstanding + 1;
    end Allocate;
    overriding procedure Deallocate (Pool : in out Heap_Pool; Storage_Address : System.Address;
        Size_In_Storage_Elements, Alignment : Count) is
    begin
        Release (Storage_Address);
        Pool.Outstanding := Pool.Outstanding - 1;
    end Deallocate;
    overriding function Storage_Size (Pool : Heap_Pool) return Count is
    begin
        return 4096;
    end Storage_Size;
    procedure Free is new Ada.Unchecked_Deallocation (Integer, Ref);
    procedure Drop (Item : in out Ref) is
    begin
        Free (Item);
    end Drop;
end Storage_Pool_Globals;
