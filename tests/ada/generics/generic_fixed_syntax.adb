procedure generic_fixed_syntax is
    generic
        type Item is delta 0.125;
    package P is
    end P;
begin
    null;
end generic_fixed_syntax;
