with Generic_Family.Secret;
package body Generic_Family.Child is
    package Private_Operations is new Generic_Family.Secret;
    function Parent_Seed return Element is
    begin
        if Private_Operations.Initial_Count < 0 then raise Program_Error; end if;
        return Private_Operations.Initial;
    end Parent_Seed;
    procedure Update (Value : in out Cell) is
    begin
        Value.Value := Next_Value (Value.Value);
        Touches := Touches + Step;
        Local_Count := Local_Count + 1;
    end Update;
    procedure Fail is
    begin
        raise Fault;
    end Fail;
    function Calls return Integer is
    begin
        return Local_Count;
    end Calls;
end Generic_Family.Child;
