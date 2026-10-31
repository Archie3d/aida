with Limited_B;
package Context_Aliases is
    package Whole renames Limited_B;
    package Nested renames Limited_B.Nested;
end Context_Aliases;
