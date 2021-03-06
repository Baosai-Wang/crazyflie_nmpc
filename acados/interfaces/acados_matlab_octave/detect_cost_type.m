%
% Copyright 2019 Gianluca Frison, Dimitris Kouzoupis, Robin Verschueren,
% Andrea Zanelli, Niels van Duijkeren, Jonathan Frey, Tommaso Sartor,
% Branimir Novoselnik, Rien Quirynen, Rezart Qelibari, Dang Doan,
% Jonas Koenemann, Yutao Chen, Tobias Schöls, Jonas Schlagenhauf, Moritz Diehl
%
% This file is part of acados.
%
% The 2-Clause BSD License
%
% Redistribution and use in source and binary forms, with or without
% modification, are permitted provided that the following conditions are met:
%
% 1. Redistributions of source code must retain the above copyright notice,
% this list of conditions and the following disclaimer.
%
% 2. Redistributions in binary form must reproduce the above copyright notice,
% this list of conditions and the following disclaimer in the documentation
% and/or other materials provided with the distribution.
%
% THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
% AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
% IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
% ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
% LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
% CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
% SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
% INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
% CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
% ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
% POSSIBILITY OF SUCH DAMAGE.;
%
%   Author: Jonathan Frey: jonathanpaulfrey(at)gmail.com

function model = detect_cost_type(model, is_e)

    import casadi.*

    x = model.sym_x;
    u = model.sym_u;

    % check type
    if strcmp(class(x(1)), 'casadi.SX')
        isSX = true;
    else
        disp('cost type detection only works for SX CasADi type!!!');
        keyboard;
    end

    if isfield(model, 'sym_z')
        z = model.sym_z;
    else
        z = SX.sym('z', 0, 0);
    end

    nx = length(x);
    nu = length(u);
    nz = length(z);

    % z = model.sym_z;
    if is_e
        expr_cost = model.cost_expr_ext_cost_e;
    else
        expr_cost = model.cost_expr_ext_cost;
    end
    cost_fun = Function('cost_fun', {x, u, z}, {expr_cost});


    if expr_cost.is_quadratic(x) && expr_cost.is_quadratic(u) && expr_cost.is_quadratic(z)
        dummy = SX.sym('dummy', 1, 1);
        
        fprintf('\n\nCost function is quadratic -> Reformulating as linear_ls cost.\n\n');

        Hxuz_fun = Function('Hxuz_fun', {dummy}, {hessian(expr_cost, [x; u; z])});
        H_xuz = full(Hxuz_fun(0));

        xuz_idx = [];
        for i = 1:(nx+nu+nz)
            if ~isempty(find(H_xuz(i,:), 1) )
                xuz_idx = union(xuz_idx, i);
            end
        end
        x_idx = intersect(1:nx, xuz_idx);
        u_idx = intersect(1+nx:nx+nu, xuz_idx);
        z_idx = intersect(1+nx+nu : nx+nu+nz, xuz_idx);

        ny = length(xuz_idx);

        Vx = zeros(ny, nx);
        Vu = zeros(ny, nu);
        Vz = zeros(ny, nz);
        W = zeros(ny);

        i = 1;
        for id = x_idx
            Vx(i, id) = 1;
            W(i, :) = H_xuz(id, xuz_idx)/2;
            i = i+1;
        end

        for id = u_idx
            iu = id - nx;
            Vu(i, iu) = 1;
            W(i, :) = H_xuz(id, xuz_idx)/2;
            i = i+1;
        end

        for id = z_idx
            Vz(i, id) = 1;
            W(i, :) = H_xuz(id, xuz_idx)/2;
            i = i+1;
        end

        xuz = [x; u; z];
        sym_y = xuz(xuz_idx);
        jac_fun = Function('jac_fun', {sym_y}, {jacobian(expr_cost, sym_y)'});
        y_ref = -W \ ( .5 * full(jac_fun(zeros(ny,1))) );

        y = -y_ref + Vx * x + Vu * u;
        if nz > 0
            y = y + Vz * z;
        end
        lls_cost_fun = Function('lls_cost_fun', {x, u, z}, {y' * W * y});

        for jj = 1:5
            x0 = rand(nx,1);
            u0 = rand(nu,1);
            z0 = rand(nz,1);

            val1 = lls_cost_fun(x0, u0, z0);
            val2 = cost_fun(x0, u0, z0);
            if norm(full(val1 - val2))> 1e-13
                disp('something went wrong when reformulating with linear least square cost');
                keyboard
            end
        end

        if is_e
            model.cost_type_e = 'linear_ls';
            model.dim_ny_e = ny;
            model.cost_Vx_e = Vx;
            model.cost_Vz_e = Vz;
            if ~isempty(find(Vu,1))
                error('Cost mayer term cannot depend on control input u!');
            end
            model.cost_W_e = W;
            model.cost_y_ref_e = y_ref;
        else
            model.cost_type = 'linear_ls';
            model.dim_ny = ny;
            model.cost_Vx = Vx;
            model.cost_Vu = Vu;
            model.cost_Vz = Vz;
            model.cost_W = W;
            model.cost_y_ref = y_ref;
        end
    % elseif 
    %  TODO: can nonlinear_ls be detected?!
    else
        fprintf('\n\nCost function is not quadratic -> Using external cost\n\n');
        if is_e
            model.cost_type_e = 'ext_cost';
        else
            model.cost_type = 'ext_cost';
        end
    end

end
