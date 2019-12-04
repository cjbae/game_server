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

-- 0 장애물
-- 1 통과

-- 맵정보 가져오기
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

-- 리스트에 노드 추가 함수
function AddList(list, node)
	list_size = #list
	list_index = list_size + 1
	list[list_index] = node
end

-- 리스트에 들어갈 노드를 만드는 함수
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

-- 리스트에서 해당 위치를 갖는 노드를 찾아 반환. 없다면 nil 반환
function FindNodeInListByPos( InList, InX, InY )
	for i, CurrNode in pairs( InList ) do
		if(CurrNode.x == InX) and (CurrNode.y == InY) then
			return CurrNode
		end
	end
	return nil
end

-- 리스트에서 가장 작은 비용 노드 반환하는 함수
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

-- 타입을 시작할때 받아와서 선공움직임은 그대로
-- 비선공움직임은 hp가 까이면 그때 공격
-- 비선공안움직임은 hp가 까이면 움직이고 공격
-- 어택쪽에서 공격범위에 있는 넘들 애드타이머 해주면 댈듯
returnY = -1
function event_player_astar_moveX(target, type)
	local openList = {}
	local closeList = {}
	
	-- 루아 배열은 + 1
	local targetX = -1 -- 어그로 없단 뜻
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

	if(x == targetX) and (y == targetY) then -- 길을 다 찾았을 때
		API_SendAttackMessage(myid, target)
		return -1
	end

	if(targetX == -1) and (targetY == -1) then
		-- 랜덤이동
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

		-- 그 자리가 움직일 수 없을때  -1 리턴
		if(map[x][y] == 1) then
			returnY = y - 1
			return x - 1
		else
			return -1
		end
	end


	-- 시작점 설정
	start_node = CreateNode(x, y, targetX, targetY)

	-- 시작점을 닫힌리스트에 추가
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
		if (x - 1 > 0) and (map[x - 1][y] == 1) then -- 움직임가능한곳이고
			if(FindNodeInListByPos(closeList, x - 1, y) == nil) then -- 닫힌거에 없으면
				tempNode = CreateNode(x - 1, y, targetX, targetY, start_node) -- 오픈리스트에 추가
				AddList(openList, tempNode)
			end
		end

		if (x + 1 < 101) and (map[x + 1][y] == 1) then -- 움직임가능한곳이고
			if(FindNodeInListByPos(closeList, x + 1, y ) == nil) then -- 닫힌거에 없으면
				tempNode = CreateNode(x + 1, y, targetX, targetY, start_node) -- 오픈리스트에 추가
				AddList(openList, tempNode)
			end
		end

		if (y - 1 > 0) and (map[x][y - 1] == 1) then -- 움직임가능한곳이고
			if(FindNodeInListByPos(closeList, x, y - 1) == nil) then -- 닫힌거에 없으면
				tempNode = CreateNode(x, y - 1, targetX, targetY, start_node) -- 오픈리스트에 추가
				AddList(openList, tempNode)
			end
		end

		if (y + 1 < 101) and (map[x][y + 1] == 1) then -- 움직임가능한곳이고
			if(FindNodeInListByPos(closeList, x, y + 1) == nil) then -- 닫힌거에 없으면
				tempNode = CreateNode(x, y + 1, targetX, targetY, start_node) -- 오픈리스트에 추가
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
	
	-- 루아 배열은 + 1
	temp_targetX = 1;
    temp_targetY = 1;

    temp_x = API_get_x(myid) + 1 -- myid
    temp_y = API_get_y(myid) + 1

	if(temp_x == temp_targetX) and (temp_y == temp_targetY) then -- 길을 다 찾았을 때
		return -1
	end

	-- 시작점 설정
	temp_start_node = CreateNode(temp_x, temp_y, temp_targetX, temp_targetY)

	-- 시작점을 닫힌리스트에 추가
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
		if (temp_x - 1 > 0) and (map[temp_x - 1][temp_y] == 1) then -- 움직임가능한곳이고
			if(FindNodeInListByPos(temp_closeList, temp_x - 1, temp_y) == nil) then -- 닫힌거에 없으면
				temp_tempNode = CreateNode(temp_x - 1, temp_y, temp_targetX, temp_targetY, temp_start_node) -- 오픈리스트에 추가
				AddList(temp_openList, temp_tempNode)
			end
		end

		if (temp_x + 1 < 101) and (map[temp_x + 1][temp_y] == 1) then -- 움직임가능한곳이고
			if(FindNodeInListByPos(temp_closeList, temp_x + 1, temp_y ) == nil) then -- 닫힌거에 없으면
				temp_tempNode = CreateNode(temp_x + 1, temp_y, temp_targetX, temp_targetY, temp_start_node) -- 오픈리스트에 추가
				AddList(temp_openList, temp_tempNode)
			end
		end

		if (temp_y - 1 > 0) and (map[temp_x][temp_y - 1] == 1) then -- 움직임가능한곳이고
			if(FindNodeInListByPos(temp_closeList, temp_x, temp_y - 1) == nil) then -- 닫힌거에 없으면
				temp_tempNode = CreateNode(temp_x, temp_y - 1, temp_targetX, temp_targetY, temp_start_node) -- 오픈리스트에 추가
				AddList(temp_openList, temp_tempNode)
			end
		end

		if (temp_y + 1 < 101) and (map[temp_x][temp_y + 1] == 1) then -- 움직임가능한곳이고
			if(FindNodeInListByPos(temp_closeList, temp_x, temp_y + 1) == nil) then -- 닫힌거에 없으면
				temp_tempNode = CreateNode(temp_x, temp_y + 1, temp_targetX, temp_targetY, temp_start_node) -- 오픈리스트에 추가
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