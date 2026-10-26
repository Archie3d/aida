procedure genericdiscriminantsyntaxerrors is
    generic
        type T (D : Integer) is new Integer;
    package G is
    end G;
begin
    null;
end genericdiscriminantsyntaxerrors;
