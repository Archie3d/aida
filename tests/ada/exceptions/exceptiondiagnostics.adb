with Ada.Text_IO; use Ada.Text_IO;
with Ada.Exceptions; use Ada.Exceptions;
with Ada.Unchecked_Deallocation;

procedure Exceptiondiagnostics is
    Original, Other : exception;
    Saved : Exception_Occurrence;
    Heap : Exception_Occurrence_Access;
    procedure Free is new Ada.Unchecked_Deallocation
        (Exception_Occurrence, Exception_Occurrence_Access);

    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error with "diagnostic regression failed";
        end if;
    end Check;

    function Contains (Text, Part : String) return Boolean is
    begin
        for I in Text'First .. Text'Last - Part'Length + 1 loop
            if Text (I .. I + Part'Length - 1) = Part then
                return True;
            end if;
        end loop;
        return False;
    end Contains;

    procedure Leaf is
    begin
        raise Original with "diagnostics";
    end Leaf;

    procedure Middle is
    begin
        Leaf;
    exception
        when E : Original =>
            Save_Occurrence (Saved, E);
            Heap := Save_Occurrence (E);
            begin
                raise Other with "nested";
            exception
                when Other => null;
            end;
            raise;
    end Middle;

    procedure Recursive (Depth : Integer) is
    begin
        if Depth = 0 then
            raise Original;
        end if;
        Recursive (Depth - 1);
    end Recursive;

    function Failed_Initializer return Integer is
    begin
        Leaf;
        return 0;
    end Failed_Initializer;

    procedure Declaration_Failure is
        Value : Integer := Failed_Initializer;
    begin
        null;
    exception
        when Original => raise Program_Error with "wrong declaration handler";
    end Declaration_Failure;

    function Early_Return return Integer is
    begin
        return 7;
    end Early_Return;
begin
    begin
        Middle;
    exception
        when E : Original =>
            declare
                Info : String := Exception_Information (E);
            begin
                Check (Info'First = 1);
                Check (Contains (Info, "raised at exceptiondiagnostics.adb:31:9"));
                Check (Contains (Info, "Ada traceback:"));
                Check (Contains (Info, "Leaf at"));
                Check (Contains (Info, "Middle at"));
                Check (Contains (Info, "Exceptiondiagnostics at"));
                Check (Info = Exception_Information (Saved));
                Check (Info = Exception_Information (Heap.all));
            end;
    end;
    Free (Heap);
    begin
        Reraise_Occurrence (Saved);
    exception
        when E : Original => Check (Exception_Information (E) = Exception_Information (Saved));
    end;
    begin
        Recursive (40);
    exception
        when E : Original =>
            Check (Contains (Exception_Information (E), "trace limited to 32 frames"));
    end;
    Check (Early_Return = 7);
    begin
        raise Other;
    exception
        when E : Other =>
            declare
                Info : String := Exception_Information (E);
            begin
                Check (not Contains (Info, "Recursive at"));
                Check (not Contains (Info, "Early_Return at"));
                Check (not Contains (Info, "Middle at"));
            end;
    end;
    begin
        Declaration_Failure;
    exception
        when E : Original =>
            Check (Contains (Exception_Information (E), "Declaration_Failure at"));
            Check (Contains (Exception_Information (E), "Failed_Initializer at"));
    end;
    -- The two predefined names share an identity, and duplicate choices
    -- within the same handler remain legal.
    Check (Numeric_Error'Identity = Constraint_Error'Identity);
    begin
        raise Numeric_Error;
    exception
        when Constraint_Error | Numeric_Error => null;
    end;
    Put_Line ("exception diagnostics ok");
exception
    when E : others =>
        Put_Line (Exception_Information (E));
        raise;
end Exceptiondiagnostics;
