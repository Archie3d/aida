with Ada.Tags.Generic_Dispatching_Constructor;
procedure DispatchingConstructorIntrinsicErrors is
    package Model is
        type Root is tagged null record;
        function Make (Params : not null access Root) return Root;
        function Create is new Ada.Tags.Generic_Dispatching_Constructor
            (Root, Root, Make);
    end Model;
    package body Model is
        function Make (Params : not null access Root) return Root is
        begin
            return (null record);
        end Make;
    end Model;
begin
    null;
end DispatchingConstructorIntrinsicErrors;
