with Ada.Text_IO; use Ada.Text_IO;
procedure AnonymousAccessArrays is
    Data : aliased String := "abc";
    type Link is access all String;
    Saved : Link;
    Result : access String;
    Calls : Integer := 0;
    procedure Capture (P : access String);
    procedure Capture (P : access String) is
        Alias : access String renames P;
    begin
        Calls := Calls + 1;
        Saved := Link (Alias);
        Result := P;
    end Capture;
    procedure Forward (P : access String) is
    begin
        Capture (P);
    end Forward;
    function Identity (P : access String) return access String is
    begin
        return P;
    end Identity;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Program_Error; end if;
    end Check;
begin
    Forward (Data'Access);
    Result (2) := 'z';
    Check (Data = "azc" and Saved.all = "azc" and Result'Length = 3 and Calls = 1);
    declare
        P : access constant String := Identity (Data'Access);
    begin
        Check (P.all = "azc" and P (1 .. 2) = "az");
    end;
    declare
        Local : aliased String := "local";
    begin
        begin
            Forward (Local'Access);
            raise Constraint_Error;
        exception
            when Program_Error => Check (Saved.all = "azc");
        end;
    end;
    Check (Result.all = "azc" and Calls = 2);
    Put_Line ("anonymous array access passed");
end AnonymousAccessArrays;
