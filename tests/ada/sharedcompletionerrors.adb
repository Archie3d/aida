with System;
procedure SharedCompletionErrors is
    value : Integer := 0;
    alias : Integer renames value;
    address : System.Address := alias'Address;
    generic
        actual : in out Integer;
    procedure Modify;
    procedure Modify is
    begin
        actual := 1;
    end Modify;
    procedure Instance is new Modify (alias);
    pragma Atomic (value);
    type Shared is new Integer;
    pragma Atomic (Shared);
    type Container is record
        part : Shared;
    end record;
    type Items is array (1 .. 2) of Shared;
    type Pointer is access Shared;
    procedure Escape (p : Pointer);
    pragma Import (C, Escape, "unsafe_pointer");
    type Pointer_Record is record
        link : Pointer;
    end record;
    procedure Nested_Escape (r : Pointer_Record);
    pragma Import (C, Nested_Escape, "unsafe_nested_pointer");
    type Stream_Pointer is access Integer;
    stream : Stream_Pointer := null;
begin
    Integer'Write (stream, alias);
    value := Integer (Shared'Input (stream));
end SharedCompletionErrors;
