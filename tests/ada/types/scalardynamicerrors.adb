procedure ScalarDynamicErrors is
    N : Integer := 3;
    Flag : Boolean := True;
    subtype Dynamic is Integer range 1 .. N;
    type Not_Static is range 0 .. Dynamic'Width;
    subtype Wrong_Real is Float range 0.0 .. Flag;
    Object_Width : Integer := N'Width;
    Extra_Argument : Integer := Dynamic'Width (1);
    type Pointer is access Integer;
    Object : Pointer := new Integer range 1 .. N;
    type Wrong_Component is record
        Value : Integer range 1 .. Flag;
    end record;
    package Deferred is
        Limit : Integer := 3;
        Value : constant Integer range 1 .. Limit;
    private
        Value : constant Integer range 1 .. Limit := 2;
    end Deferred;
begin
    null;
end ScalarDynamicErrors;
