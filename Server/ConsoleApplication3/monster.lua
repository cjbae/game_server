myid = 99999;  
move_count = 0;

function set_uid(x) 
	myid = x;  
end

function event_player_move(player)
   player_x = API_get_x(player);
   player_y = API_get_y(player);
   my_x = API_get_x(myid);
   my_y = API_get_y(myid);
   if (player_x == my_x) then
      if (player_y == my_y) then
	     API_SendMessage(myid, player, "HELLO");
	  end
   end
end


-----------------------------------------------------------------------------------------------------------------

-- 0 ��ֹ�
-- 1 ���

-- ������ ��������
map_size = 100
map = {}
for i = 1, map_size do
map[i] = {}
	for i = 1, map_size do
	end
end

function set_map(x, y, info)
	map[x + 1][y + 1] = info
end

nearList = {}

function set_near(id)
	list_size = #nearList
	list_index = list_size + 1
	nearList[list_index] = id
end

-- ����Ʈ�� ��� �߰� �Լ�
function AddList(list, node)
	list_size = #list
	list_index = list_size + 1
	list[list_index] = node
end

-- ����Ʈ�� �� ��带 ����� �Լ�
function CreateNode( x, y, tX, tY, parent )

	Object = {}
	Object.x = x
	Object.y = y
	Object.g = 0
	Object.h = 0
	Object.f = 0
	Object.parent = parent

	if(Object.parent ~= nil) then
		Object.g = 10 + Object.parent.f
		Object.h = math.abs( Object.x - tX ) + math.abs( Object.y - tY )
		Object.f = Object.g + Object.h
	end
	
	return Object
end

-- ����Ʈ���� �ش� ��ġ�� ���� ��带 ã�� ��ȯ. ���ٸ� nil ��ȯ
function FindNodeInListByPos( InList, InX, InY )
	for i, CurrNode in pairs( InList ) do
		if(CurrNode.x == InX) and (CurrNode.y == InY) then
			return CurrNode
		end
	end
	return nil
end

-- ����Ʈ���� ���� ���� ��� ��� ��ȯ�ϴ� �Լ�
function LowCostNode(InList)
	local tempcost = 99999
	local temp = {}
	for i, CurrNode in pairs(InList) do
		if(CurrNode.f <= tempcost) then
			tempcost = CurrNode.f
			temp = CurrNode
		end
	end
	return temp
end

function Aggro_InRange(x, y, target)
	local tx = API_get_x(target) + 1
    local ty = API_get_y(target) + 1
	dist = (x - tx) * (x - tx) + (y - ty) * (y - ty)
	if(dist <= 3 * 3) then
		return true
	else
		return false
	end
	
end

-- Ÿ���� �����Ҷ� �޾ƿͼ� ������������ �״��
-- �񼱰��������� hp�� ���̸� �׶� ����
-- �񼱰��ȿ������� hp�� ���̸� �����̰� ����
-- �����ʿ��� ���ݹ����� �ִ� �ѵ� �ֵ�Ÿ�̸� ���ָ� ���
returnY = -1
function event_player_astar_moveX(target, type)
	local openList = {}
	local closeList = {}
	
	-- ��� �迭�� + 1
	local targetX = -1 -- ��׷� ���� ��
    local targetY = -1
    local x = API_get_x(myid) + 1 -- myid
    local y = API_get_y(myid) + 1


	--if(Aggro_InRange(x, y, target) == true) then
	--	targetX = API_get_x(target) + 1
	--	targetY = API_get_y(target) + 1
	--end

	if(type == 0) then
		if(target ~= -1) then
			targetX = API_get_x(target) + 1
			targetY = API_get_y(target) + 1
		end
	elseif(type == 1) then
		if(target ~= -1) then
			targetX = API_get_x(target) + 1
			targetY = API_get_y(target) + 1
		end
	else
		if(target ~= -1) then
			targetX = API_get_x(target) + 1
			targetY = API_get_y(target) + 1
		else
			return -1
		end
	end

	if(x == targetX) and (y == targetY) then -- ���� �� ã���� ��
		API_SendAttackMessage(myid, target)
		return -1
	end

	if(targetX == -1) and (targetY == -1) then
		-- �����̵�
		RandomN = math.random(1, 4)
		if(RandomN == 1) then
			x = x + 1
		elseif(RandomN == 2) then
			x = x - 1
		elseif(RandomN == 3) then
			y = y + 1
		else
			y = y - 1
		end

		-- �� �ڸ��� ������ �� ������  -1 ����
		if(map[x][y] == 1) then
			returnY = y - 1
			return x - 1
		else
			return -1
		end
	end


	-- ������ ����
	start_node = CreateNode(x, y, targetX, targetY)

	-- �������� ��������Ʈ�� �߰�
	AddList(closeList, start_node)

	on = 0
	while on ~= 1000 do
		x = closeList[#closeList].x
		y = closeList[#closeList].y

		if(#openList ~= 0) then
			for i = #openList, 1, -1 do
				openList[i] = nil
				table.remove(openList, i)
			end
		end

		tempNode = {}
		if (x - 1 > 0) and (map[x - 1][y] == 1) then -- �����Ӱ����Ѱ��̰�
			if(FindNodeInListByPos(closeList, x - 1, y) == nil) then -- �����ſ� ������
				tempNode = CreateNode(x - 1, y, targetX, targetY, start_node) -- ���¸���Ʈ�� �߰�
				AddList(openList, tempNode)
			end
		end

		if (x + 1 < 101) and (map[x + 1][y] == 1) then -- �����Ӱ����Ѱ��̰�
			if(FindNodeInListByPos(closeList, x + 1, y ) == nil) then -- �����ſ� ������
				tempNode = CreateNode(x + 1, y, targetX, targetY, start_node) -- ���¸���Ʈ�� �߰�
				AddList(openList, tempNode)
			end
		end

		if (y - 1 > 0) and (map[x][y - 1] == 1) then -- �����Ӱ����Ѱ��̰�
			if(FindNodeInListByPos(closeList, x, y - 1) == nil) then -- �����ſ� ������
				tempNode = CreateNode(x, y - 1, targetX, targetY, start_node) -- ���¸���Ʈ�� �߰�
				AddList(openList, tempNode)
			end
		end

		if (y + 1 < 101) and (map[x][y + 1] == 1) then -- �����Ӱ����Ѱ��̰�
			if(FindNodeInListByPos(closeList, x, y + 1) == nil) then -- �����ſ� ������
				tempNode = CreateNode(x, y + 1, targetX, targetY, start_node) -- ���¸���Ʈ�� �߰�
				AddList(openList, tempNode)
			end
		end

		tem = LowCostNode(openList)
		AddList(closeList, tem)

		for i, CurrNode in pairs(openList) do
			if(CurrNode.x == targetX) and (CurrNode.y == targetY) then
				returnY = closeList[2].y - 1
				return closeList[2].x - 1
			end
		end

		if(#openList == 0) then
			if(#closeList ~= 1) then
				returnY = closeList[2].y - 1
				return closeList[2].x - 1
			end
		end
		on = on + 1
	end
	return -1
end

function event_player_astar_moveY(target, type)
	return returnY
end



temp_returnY = -1
function temp_event_player_astar_moveX()
	temp_openList = {}
	temp_closeList = {}
	
	-- ��� �迭�� + 1
	temp_targetX = 1;
    temp_targetY = 1;

    temp_x = API_get_x(myid) + 1 -- myid
    temp_y = API_get_y(myid) + 1

	if(temp_x == temp_targetX) and (temp_y == temp_targetY) then -- ���� �� ã���� ��
		return -1
	end

	-- ������ ����
	temp_start_node = CreateNode(temp_x, temp_y, temp_targetX, temp_targetY)

	-- �������� ��������Ʈ�� �߰�
	AddList(temp_closeList, temp_start_node)

	temp_on = 0
	while temp_on ~= 1000 do
		temp_x = temp_closeList[#temp_closeList].x
		temp_y = temp_closeList[#temp_closeList].y

		if(#temp_openList ~= 0) then
			for i = #temp_openList, 1, -1 do
				temp_openList[i] = nil
				table.remove(temp_openList, i)
			end
		end

		temp_tempNode = {}
		if (temp_x - 1 > 0) and (map[temp_x - 1][temp_y] == 1) then -- �����Ӱ����Ѱ��̰�
			if(FindNodeInListByPos(temp_closeList, temp_x - 1, temp_y) == nil) then -- �����ſ� ������
				temp_tempNode = CreateNode(temp_x - 1, temp_y, temp_targetX, temp_targetY, temp_start_node) -- ���¸���Ʈ�� �߰�
				AddList(temp_openList, temp_tempNode)
			end
		end

		if (temp_x + 1 < 101) and (map[temp_x + 1][temp_y] == 1) then -- �����Ӱ����Ѱ��̰�
			if(FindNodeInListByPos(temp_closeList, temp_x + 1, temp_y ) == nil) then -- �����ſ� ������
				temp_tempNode = CreateNode(temp_x + 1, temp_y, temp_targetX, temp_targetY, temp_start_node) -- ���¸���Ʈ�� �߰�
				AddList(temp_openList, temp_tempNode)
			end
		end

		if (temp_y - 1 > 0) and (map[temp_x][temp_y - 1] == 1) then -- �����Ӱ����Ѱ��̰�
			if(FindNodeInListByPos(temp_closeList, temp_x, temp_y - 1) == nil) then -- �����ſ� ������
				temp_tempNode = CreateNode(temp_x, temp_y - 1, temp_targetX, temp_targetY, temp_start_node) -- ���¸���Ʈ�� �߰�
				AddList(temp_openList, temp_tempNode)
			end
		end

		if (temp_y + 1 < 101) and (map[temp_x][temp_y + 1] == 1) then -- �����Ӱ����Ѱ��̰�
			if(FindNodeInListByPos(temp_closeList, temp_x, temp_y + 1) == nil) then -- �����ſ� ������
				temp_tempNode = CreateNode(temp_x, temp_y + 1, temp_targetX, temp_targetY, temp_start_node) -- ���¸���Ʈ�� �߰�
				AddList(temp_openList, temp_tempNode)
			end
		end

		tem = LowCostNode(temp_openList)
		AddList(temp_closeList, tem)

		for i, CurrNode in pairs(temp_openList) do
			if(CurrNode.x == temp_targetX) and (CurrNode.y == temp_targetY) then
				temp_returnY = temp_closeList[2].y - 1
				return temp_closeList[2].x - 1
			end
		end

		if(#temp_openList == 0) then
			if(#temp_closeList ~= 1) then
				temp_returnY = temp_closeList[2].y - 1
				return temp_closeList[2].x - 1
			end
		end

		temp_on = temp_on + 1
	end
	return 2424
end

function temp_event_player_astar_moveY()
	return temp_returnY
end