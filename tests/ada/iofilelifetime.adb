with Ada.Text_IO; use Ada.Text_IO;
with Ada.Sequential_IO;
with Ada.Direct_IO;
with Ada.Streams.Stream_IO;
with Ada.Unchecked_Deallocation;
procedure IOFileLifetime is
    package S is new Ada.Sequential_IO (Integer);
    package D is new Ada.Direct_IO (Integer);
    package T renames Ada.Streams.Stream_IO;
    type Group is limited record
        text : File_Type;
        sequential : S.File_Type;
        direct : D.File_Type;
        stream : T.File_Type;
    end record;
    type Group_Access is access Group;
    procedure Free is new Ada.Unchecked_Deallocation (Group, Group_Access);
    outer : File_Type;
    function New_File return File_Type is
    begin
        return result : File_Type do
            Create (result);
        end return;
    end New_File;
    function Selected return File_Type is
    begin
        return Current_Output;
    end Selected;
begin
    Create (outer);
    Set_Output (outer);
    declare
        saved : constant File_Type := Selected;
    begin
        Put (saved, "borrowed");
    end;
    if not Is_Open (outer) then
        raise Program_Error with "borrowed result closed owner";
    end if;
    Set_Output (Standard_Output);
    Close (outer);
    for i in 1 .. 100 loop
        declare
            returned : File_Type := New_File;
            files : Group;
            heap : Group_Access := new Group;
        begin
            if not Is_Open (returned) then
                raise Program_Error;
            end if;
            Create (files.text);
            S.Create (files.sequential);
            D.Create (files.direct);
            T.Create (files.stream);
            Create (heap.text);
            S.Create (heap.sequential);
            D.Create (heap.direct);
            T.Create (heap.stream);
            Free (heap);
            if i mod 2 = 0 then
                raise Constraint_Error;
            end if;
        exception
            when Constraint_Error => null;
        end;
    end loop;
    declare
        files : array (1 .. 10) of File_Type;
    begin
        for i in files'Range loop
            Create (files (i));
        end loop;
    end;
    Create (outer);
    Close (outer);
    Put_Line ("iofilelifetime: passed");
end IOFileLifetime;
