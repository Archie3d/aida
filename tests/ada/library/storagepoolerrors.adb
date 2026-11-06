with System;
with System.Storage_Pools;
with System.Storage_Elements; use System.Storage_Elements;
procedure StoragePoolErrors is
    type Pool_Type is new System.Storage_Pools.Root_Storage_Pool with null record;
    overriding procedure Allocate (Pool : in out Pool_Type; Storage_Address : out System.Address;
        Size_In_Storage_Elements, Alignment : Storage_Count);
    overriding procedure Deallocate (Pool : in out Pool_Type; Storage_Address : System.Address;
        Size_In_Storage_Elements, Alignment : Storage_Count);
    overriding function Storage_Size (Pool : Pool_Type) return Storage_Count;
    overriding procedure Allocate (Pool : in out Pool_Type; Storage_Address : out System.Address;
        Size_In_Storage_Elements, Alignment : Storage_Count) is
    begin
        raise Storage_Error;
    end Allocate;
    overriding procedure Deallocate (Pool : in out Pool_Type; Storage_Address : System.Address;
        Size_In_Storage_Elements, Alignment : Storage_Count) is null;
    overriding function Storage_Size (Pool : Pool_Type) return Storage_Count is
    begin
        return 0;
    end Storage_Size;
    Pool : Pool_Type;
    type Ref is access Integer;
    for Ref'Storage_Pool use 123;
    type Shared is access Integer;
    for Shared'Storage_Pool use Pool;
    for Shared'Storage_Pool use Pool;
    for Shared'Storage_Size use 0;
    type Zero is access Integer;
    for Zero'Storage_Size use 0;
    for Zero'Storage_Pool use Pool;
    type Zero_Child is new Zero;
    Illegal : Zero_Child := new Integer;
    type Derived is new Shared;
    for Derived'Storage_Pool use Pool;
    subtype View is Shared;
    for View'Storage_Pool use Pool;
    type Callback is access procedure;
    for Callback'Storage_Pool use Pool;
    type Late is access Integer;
    Value : Late := new Integer;
    for Late'Storage_Pool use Pool;
    Not_Access : Integer := 0;
    Bad : constant Storage_Count := Not_Access'Storage_Size;
    Bad2 : constant Storage_Count := Integer'Storage_Size;
    type Limited_Root is abstract new System.Storage_Pools.Root_Storage_Pool with null record;
    Abstract_Pool : Limited_Root;
    type Holder (Item : access Integer) is limited null record;
    type Holder_Ref is access Holder;
    for Holder_Ref'Storage_Pool use Pool;
    Coextension : Holder_Ref := new Holder (new Integer'(7));
    procedure Constant_View (Item : Pool_Type) is
        type Ref is access Integer;
        for Ref'Storage_Pool use Item;
    begin
        null;
    end Constant_View;
begin
    null;
end StoragePoolErrors;
