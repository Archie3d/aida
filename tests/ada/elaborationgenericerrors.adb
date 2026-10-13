procedure Elaborationgenericerrors is
    generic
    function Missing return Integer;
    function Instance is new Missing;
    generic
    package Missing_Package is
        function Get return Integer;
    end Missing_Package;
    package Package_Instance is new Missing_Package;
begin
    null;
end Elaborationgenericerrors;
