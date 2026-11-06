with Ada.Finalization;
package System.Storage_Pools is
    type Root_Storage_Pool is abstract new Ada.Finalization.Limited_Controlled with null record;
    procedure Allocate (Pool : in out Root_Storage_Pool) is abstract;
end System.Storage_Pools;
