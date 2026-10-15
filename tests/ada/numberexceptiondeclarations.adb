with Ada.Text_IO; use Ada.Text_IO;

procedure NumberExceptionDeclarations is
    Count : constant := 1;
    Failure : exception;
    package Names is
        Count : constant := 99;
        Failure : exception;
    end Names;
    procedure Check is
        use Names;
        Count, Other : constant := 2;
        Failure, Other_Failure : exception;
        Alias : exception renames Failure;
    begin
        if Count /= 2 or Other /= 2 or Names.Count /= 99 then
            raise Program_Error;
        end if;
        begin
            raise Alias;
        exception
            when Failure => Put_Line ("local exception");
        end;
        begin
            raise Other_Failure;
        exception
            when Other_Failure => Put_Line ("grouped exception");
        end;
        begin
            raise Names.Failure;
        exception
            when Failure => raise Program_Error;
            when Names.Failure => Put_Line ("package exception");
        end;
    end Check;
begin
    Check;
    if Count /= 1 then
        raise Program_Error;
    end if;
    raise Failure;
exception
    when Failure => Put_Line ("outer exception");
end NumberExceptionDeclarations;
