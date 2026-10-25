with Enclosing_Model;
procedure EnclosingNameErrors is
    Result : Integer := 0;
    procedure Finished (Parameter : Integer) is
        Local : Integer := Parameter;
    begin
        null;
    end Finished;
    procedure Alias (Parameter : Integer) renames Finished;
    procedure Overloaded (Value : Integer) is
        Only_Outer : Integer := 1;
        procedure Overloaded (Value : Boolean) is
        begin
            Result := Overloaded.Only_Outer;
        end Overloaded;
    begin
        Overloaded (True);
    end Overloaded;
    generic
        Number : Integer;
    package Template is
        Visible : Integer := Number;
    end Template;
    package Instance is new Template (1);
begin
    Region : declare
        Local : Integer := 1;
    begin
        Result := Region;
        Result := EnclosingNameErrors.Region;
        Result := Region.Missing;
        Result := Region.Result;
        Loop_Name : for Index in 1 .. 2 loop
            Loop_Name.Index := 3;
        end loop Loop_Name;
    end Region;
    Result := Region.Local;
    Result := Finished.Local;
    Result := Alias.Parameter;
    Result := Instance.Number;
    Result := Enclosing_Model.Body_Value;
    <<Plain_Label>>
    Result := Plain_Label;
    Result := Plain_Label.Local;
    declare
        Region : Integer := 1;
    begin
        Result := Region.Local;
    end;
end EnclosingNameErrors;
