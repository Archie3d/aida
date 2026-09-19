with Ada.Text_IO; use Ada.Text_IO;
with Ada.Exceptions; use Ada.Exceptions;
with Ada.Unchecked_Deallocation;

procedure Savedoccurrences is
    Original, Other : exception;
    Saved : Exception_Occurrence;
    type Wrapper is record
        Item : Exception_Occurrence;
    end record;
    Box : Wrapper;
    Heap : Exception_Occurrence_Access;
    procedure Free is new Ada.Unchecked_Deallocation
        (Exception_Occurrence, Exception_Occurrence_Access);

    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error with "saved occurrence regression failed";
        end if;
    end Check;

    procedure Produce (Length : Integer) is
        Message : String (5 .. Length + 4) := (others => 'x');
    begin
        if Length > 10 then
            Message (10) := Character'Val (0);
        end if;
        raise Original with Message;
    exception
        when E : Original =>
            Save_Occurrence (Saved, E);
            Heap := Save_Occurrence (E);
    end Produce;

    procedure Forward (Target : out Exception_Occurrence) is
    begin
        Save_Occurrence (Target, Heap.all);
    end Forward;
begin
    Check (Exception_Identity (Saved) = Null_Id);
    -- Both saves must outlive the source handler, its message, and its frame.
    for Length in 199 .. 201 loop
        Produce (Length);
        declare
            Full : String := Exception_Message (Heap.all);
            Short : String := Exception_Message (Saved);
        begin
            Check (Full'Length = Length and Full'First = 1);
            if Length <= 200 then
                Check (Short'Length = Length);
            else
                Check (Short'Length = 200);
            end if;
            Check (Short = Full (1 .. Short'Length));
            Check (Full (6) = Character'Val (0));
            Save_Occurrence (Saved, Saved);
            Save_Occurrence (Heap.all, Heap.all);
            Check (Exception_Message (Heap.all) = Full);
            Check (Exception_Message (Saved) = Short);
        end;
        Free (Heap);
        Check (Heap = null);
        Check (Exception_Identity (Saved) = Original'Identity);
        begin
            Reraise_Occurrence (Saved);
        exception
            when E : Original =>
                Check (Exception_Message (E) = Exception_Message (Saved));
        end;
    end loop;
    Produce (5000);
    Forward (Saved);
    Check (Exception_Message (Heap.all)'Length = 5000);
    Check (Exception_Message (Saved)'Length = 200);
    -- Overwrite a heap result; its original trailing storage is still freed
    -- with the same allocation, while the replacement uses inline storage.
    Save_Occurrence (Heap.all, Saved);
    Check (Exception_Message (Heap.all) = Exception_Message (Saved));
    Free (Heap);
    begin
        raise Other with "replacement";
    exception
        when E : Other => Save_Occurrence (Saved, E);
    end;
    Check (Exception_Information (Saved) = "OTHER: replacement");
    Save_Occurrence (Box.Item, Saved);
    Save_Occurrence (Saved, Null_Occurrence);
    Check (Exception_Information (Box.Item) = "OTHER: replacement");
    Save_Occurrence (Saved, Box.Item);
    Heap := Save_Occurrence (Saved);
    Save_Occurrence (Saved, Null_Occurrence);
    Check (Exception_Identity (Saved) = Null_Id);
    Check (Exception_Message (Heap.all) = "replacement");
    Free (Heap);
    Heap := Save_Occurrence (Null_Occurrence);
    Check (Heap /= null and Exception_Identity (Heap.all) = Null_Id);
    Reraise_Occurrence (Heap.all);
    Free (Heap);
    Free (Heap);
    Produce (0);
    Check (Exception_Message (Saved) = "");
    Check (Exception_Message (Heap.all) = "");
    Free (Heap);
    Put_Line ("saved occurrences ok");
end Savedoccurrences;
