package body Repack_Operations is
    function Make (Left, Right : Integer) return Parent is
    begin
        return (Left, Right);
    end Make;
    function Measure (Item : Parent) return Integer is
    begin
        return 100 * Item.Left + Item.Right;
    end Measure;
    procedure Update (Item : in out Parent) is
    begin
        Item := (Item.Right, Item.Left + 1);
    end Update;
    procedure Initialize (Item : out Parent) is
    begin
        Item := (11, 12);
    end Initialize;
    procedure Fail (Item : in out Parent) is
    begin
        Item := (22, 23);
        raise Constraint_Error;
    end Fail;
    function Load (Stream : access Ada.Streams.Root_Stream_Type'Class) return Parent is
        Left : Integer := Integer'Input (Stream);
        Right : Integer := Integer'Input (Stream);
    begin
        return (Left, Right);
    end Load;
    procedure Read (Stream : access Ada.Streams.Root_Stream_Type'Class; Item : out Parent) is
    begin
        Item := Load (Stream);
    end Read;
    procedure Switch (Item : in out Choice) is
    begin
        Item := (True, Item.X + 1);
    end Switch;
end Repack_Operations;
