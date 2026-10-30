package Categorized_Types is
    pragma Preelaborate (Library_Unit => Categorized_Types);
    type Value is private;
    pragma Preelaborable_Initialization (Value);
    type Opaque is private;
    protected type Lock is
        procedure Touch;
    private
        Count : Integer := 0;
    end Lock;
    pragma Preelaborable_Initialization (Lock);
    function Get (Item : Value) return Integer;
private
    type Value is record
        Count : Integer := 17;
    end record;
    type Opaque is record
        Count : Integer := 0;
    end record;
end Categorized_Types;
